#include "d3d12_renderer.hpp"

#include <fstream> // temp

#include "../util/defines.hpp"
#include "../util/log.hpp"
#include "../scene_graph/scene_graph.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../window.hpp"

#include "d3d12_defines.hpp"
#include "d3d12_resource_pool.hpp"
#include "d3d12_functions.hpp"

LINK_NODE_FUNCTION(wr::D3D12RenderSystem, wr::MeshNode, Init_MeshNode, Render_MeshNode)
LINK_NODE_FUNCTION(wr::D3D12RenderSystem, wr::AnimNode, Init_AnimNode, Render_AnimNode)

namespace wr
{
	D3D12RenderSystem::~D3D12RenderSystem()
	{
		PerfOutput_Framerate();

		d3d12::Destroy(m_cb_heap);
		d3d12::Destroy(m_device);
		d3d12::Destroy(m_direct_queue);
		d3d12::Destroy(m_copy_queue);
		d3d12::Destroy(m_compute_queue);
		if (m_render_window.has_value()) d3d12::Destroy(m_render_window.value());
	}

	void D3D12RenderSystem::Init(std::optional<Window*> window)
	{
		auto heapSize = 32;
		auto heapAlignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		auto x = (heapSize / heapAlignment + ((heapSize % heapAlignment) != 0 ? 1 : 0))*(heapAlignment);

		m_device = d3d12::CreateDevice();
		m_direct_queue = d3d12::CreateCommandQueue(m_device, d3d12::CmdListType::CMD_LIST_DIRECT);

		if (window.has_value())
		{
			m_render_window = d3d12::CreateRenderWindow(m_device, window.value()->GetWindowHandle(), m_direct_queue, d3d12::settings::num_back_buffers);
		}

		// Create fences
		for (auto i = 0; i < m_fences.size(); i++)
		{
			m_fences[i] = d3d12::CreateFence(m_device);
		}

		// Temporary
		// Create Constant Buffer Heap
		constexpr auto sbo_size = SizeAlign(sizeof(temp::ConstantBufferData), 256) * d3d12::settings::num_back_buffers;
		constexpr auto bbo_size = SizeAlign(sizeof(temp::ConstantBufferData), 65536) * d3d12::settings::num_back_buffers;
		m_cb_heap = d3d12::CreateHeap_SBO(m_device, sbo_size, d3d12::ResourceType::BUFFER, d3d12::settings::num_back_buffers);
		//m_cb_heap = d3d12::CreateHeap_BBO(m_device, bbo_size, d3d12::ResourceType::BUFFER, d3d12::settings::num_back_buffers);

		// Create Constant Buffer
		m_cb = d3d12::AllocConstantBuffer(m_cb_heap, sizeof(temp::ConstantBufferData));
		d3d12::MapHeap(m_cb_heap);

		// Load Shaders.
		m_vertex_shader = d3d12::LoadShader(d3d12::ShaderType::VERTEX_SHADER, "basic.hlsl", "main_vs");
		m_pixel_shader = d3d12::LoadShader(d3d12::ShaderType::PIXEL_SHADER, "basic.hlsl", "main_ps");

		// Create Root Signature
		CD3DX12_DESCRIPTOR_RANGE  desc_table_ranges;
		desc_table_ranges.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0);

		d3d12::desc::RootSignatureDesc rs_desc;
		rs_desc.m_samplers.push_back({ d3d12::TextureFilter::FILTER_LINEAR, d3d12::TextureAddressMode::TAM_MIRROR });
		rs_desc.m_parameters.resize(1);
		rs_desc.m_parameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

		m_root_signature = d3d12::CreateRootSignature(rs_desc);
		d3d12::FinalizeRootSignature(m_root_signature, m_device);

		// Create Pipeline State
		d3d12::desc::PipelineStateDesc pso_desc;
		pso_desc.m_dsv_format = d3d12::Format::UNKNOWN;
		pso_desc.m_num_rtv_formats = 1;
		pso_desc.m_rtv_formats[0] = d3d12::Format::R8G8B8A8_UNORM;
		pso_desc.m_input_layout = temp::Vertex::GetInputLayout();

		m_pipeline_state = d3d12::CreatePipelineState();
		d3d12::SetVertexShader(m_pipeline_state, m_vertex_shader);
		d3d12::SetFragmentShader(m_pipeline_state, m_pixel_shader);
		d3d12::SetRootSignature(m_pipeline_state, m_root_signature);
		d3d12::FinalizePipeline(m_pipeline_state, m_device, pso_desc);

		// Create viewport
		m_viewport = d3d12::CreateViewport(window.has_value() ? window.value()->GetWidth() : 400, window.has_value() ? window.value()->GetHeight() : 400);

		// Create screen quad
		m_vertex_buffer = d3d12::CreateStagingBuffer(m_device, (void*)temp::quad_vertices, 4 * sizeof(temp::Vertex), sizeof(temp::Vertex), d3d12::ResourceState::VERTEX_AND_CONSTANT_BUFFER);

		// Create Command List
		m_direct_cmd_list = d3d12::CreateCommandList(m_device, d3d12::settings::num_back_buffers, d3d12::CmdListType::CMD_LIST_DIRECT);

		// Begin Recording
		auto frame_idx = m_render_window.has_value() ? m_render_window.value()->m_frame_idx : 0;
		d3d12::Begin(m_direct_cmd_list, frame_idx);

		// Stage screen quad
		d3d12::StageBuffer(m_vertex_buffer, m_direct_cmd_list);

		// Execute
		d3d12::End(m_direct_cmd_list);
		d3d12::Execute(m_direct_queue, { m_direct_cmd_list }, m_fences[frame_idx]);
		m_fences[frame_idx]->m_fence_value++;
		d3d12::Signal(m_fences[frame_idx], m_direct_queue);

		// Start counting the framerate.
		prev = std::chrono::high_resolution_clock::now();
	}

	std::unique_ptr<Texture> D3D12RenderSystem::Render(std::shared_ptr<SceneGraph> const & scene_graph, fg::FrameGraph& frame_graph)
	{
		UpdateFramerate();

		using recursive_func_t = std::function<void(std::shared_ptr<Node>)>;
		recursive_func_t recursive_render = [this, &recursive_render](std::shared_ptr<Node> const & parent)
		{
			for (auto & node : parent->m_children)
			{
				node->Render(this, node.get());
				recursive_render(node);
			}
		};

		frame_graph.Execute(*this, *scene_graph.get());

		recursive_render(scene_graph->GetRootNode());

		return std::unique_ptr<Texture>();
	}

	void D3D12RenderSystem::Resize(std::int32_t width, std::int32_t height)
	{
	}

	std::shared_ptr<MaterialPool> D3D12RenderSystem::CreateMaterialPool(std::size_t size_in_mb)
	{
		return std::make_shared<D3D12MaterialPool>(size_in_mb);
	}

	std::shared_ptr<ModelPool> D3D12RenderSystem::CreateModelPool(std::size_t size_in_mb)
	{
		return std::make_shared<D3D12ModelPool>(size_in_mb);
	}

	void D3D12RenderSystem::Init_MeshNode(MeshNode* node)
	{
		//LOG("init mesh");
	}

	void D3D12RenderSystem::Init_AnimNode(AnimNode* node)
	{
		//LOG("init anim");
	}

	void D3D12RenderSystem::Render_MeshNode(MeshNode* node)
	{
		//LOG("render mesh");
	}

	void D3D12RenderSystem::Render_AnimNode(AnimNode* node)
	{
		//LOG("render anim");
	}

	unsigned int D3D12RenderSystem::GetFrameIdx()
	{
		if (m_render_window.has_value())
		{
			return m_render_window.value()->m_frame_idx;
		}
		else
		{
			LOGW("Called `D3D12RenderSystem::GetFrameIdx` without a window!");
			return 0;
		}
	}

	d3d12::RenderWindow* D3D12RenderSystem::GetRenderWindow()
	{
		if (m_render_window.has_value())
		{
			return m_render_window.value();
		}
		else
		{
			LOGW("Called `D3D12RenderSystem::GetRenderWindow` without a window!");
			return nullptr;
		}
	}

	void D3D12RenderSystem::UpdateFramerate()
	{
		frames++;
		auto now = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> diff = now - prev;

		if (diff.count() >= 1)
		{
			prev = std::chrono::high_resolution_clock::now();
			framerate = frames;
			captured_framerates.push_back(framerate);
			frames = 0;
		}
	}

	void D3D12RenderSystem::PerfOutput_Framerate()
	{
		std::ofstream file;
		file.open("perf_framerate.txt");

		// Calc average fps
		int fps_sum = 0;
		for (auto fps : captured_framerates)
			fps_sum += fps;
		int average_fps = fps_sum / captured_framerates.size();

		// Calc average frame time
		/*int frametime_sum = 0;
		for (auto frametime : captured_frametimes)
			frametime_sum += frametime;
		int average_frametime = frametime_sum / captured_frametimes.size();*/

		//file << "Average frametime over " << captured_frametimes.size() << ": " << average_frametime << '\n';
		file << "Average framerate over " << captured_framerates.size() << ": " << average_fps << '\n';

		for (auto fps : captured_framerates)
		{
			file << fps << '\n';
		}

		file.close();
	}

} /*  */
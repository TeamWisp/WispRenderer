#include "d3d12_renderer.hpp"

#include "../util/defines.hpp"
#include "../util/log.hpp"
#include "../scene_graph/scene_graph.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../window.hpp"

#include "d3d12_resource_pool.hpp"
#include "d3d12_functions.hpp"

LINK_NODE_FUNCTION(wr::D3D12RenderSystem, wr::MeshNode, Init_MeshNode, Render_MeshNode)
LINK_NODE_FUNCTION(wr::D3D12RenderSystem, wr::AnimNode, Init_AnimNode, Render_AnimNode)

namespace temp
{

	struct Vertex
	{
		float m_pos[3];

		static std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputLayout()
		{
			std::vector<D3D12_INPUT_ELEMENT_DESC> layout = {
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, m_pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			return layout;
		}
	};

	static const constexpr float size = 0.5f;
	static const constexpr Vertex quad_vertices[] = {
		{ -size, -size, 0.f },
		{ size, -size, 0.f },
		{ -size, size, 0.f },
		{ size, size, 0.f },
	};

} /* temp */

namespace wr
{
	D3D12RenderSystem::~D3D12RenderSystem()
	{
		d3d12::Destroy(m_device);
		d3d12::Destroy(m_direct_queue);
		d3d12::Destroy(m_copy_queue);
		d3d12::Destroy(m_compute_queue);
		if (m_render_window.has_value()) d3d12::Destroy(m_render_window.value());
	}

	void D3D12RenderSystem::Init(std::optional<Window*> window)
	{
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
		// Load Shaders.
		m_vertex_shader = d3d12::LoadShader(d3d12::ShaderType::VERTEX_SHADER, "basic.hlsl", "main_vs");
		m_pixel_shader = d3d12::LoadShader(d3d12::ShaderType::PIXEL_SHADER, "basic.hlsl", "main_ps");

		// Create Root Signature
		CD3DX12_DESCRIPTOR_RANGE  desc_table_ranges;
		desc_table_ranges.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0);

		d3d12::desc::RootSignatureDesc rs_desc;
		rs_desc.m_samplers.push_back({ d3d12::TextureFilter::FILTER_LINEAR, d3d12::TextureAddressMode::TAM_MIRROR });

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
	}

	std::unique_ptr<Texture> D3D12RenderSystem::Render(std::shared_ptr<SceneGraph> const & scene_graph, fg::FrameGraph& frame_graph)
	{
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
		LOG("init mesh");
	}

	void D3D12RenderSystem::Init_AnimNode(AnimNode* node)
	{
		LOG("init anim");
	}

	void D3D12RenderSystem::Render_MeshNode(MeshNode* node)
	{
		LOG("render mesh");
	}

	void D3D12RenderSystem::Render_AnimNode(AnimNode* node)
	{
		LOG("render anim");
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

} /*  */
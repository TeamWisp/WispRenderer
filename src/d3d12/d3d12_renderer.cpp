#include "d3d12_renderer.hpp"

#include "../util/defines.hpp"
#include "../util/log.hpp"
#include "../scene_graph/scene_graph.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../window.hpp"

#include "d3d12_defines.hpp"
#include "d3d12_resource_pool.hpp"
#include "d3d12_functions.hpp"
#include "d3d12_pipeline_registry.hpp"
#include "d3d12_shader_registry.hpp"
#include "d3d12_root_signature_registry.hpp"

#include "../scene_graph/mesh_node.hpp"
#include "../scene_graph/camera_node.hpp"

LINK_NODE_FUNCTION(wr::D3D12RenderSystem, wr::MeshNode, Init_MeshNode, Update_MeshNode, Render_MeshNode)
LINK_NODE_FUNCTION(wr::D3D12RenderSystem, wr::CameraNode, Init_CameraNode, Update_CameraNode, Render_CameraNode)

namespace wr
{
	D3D12RenderSystem::~D3D12RenderSystem()
	{
		// wait for end
		for (auto& fence : m_fences)
		{
			d3d12::WaitFor(fence);
		}

		d3d12::Destroy(m_cb_heap);
		d3d12::Destroy(m_device);
		d3d12::Destroy(m_direct_queue);
		d3d12::Destroy(m_copy_queue);
		d3d12::Destroy(m_compute_queue);
		if (m_render_window.has_value()) d3d12::Destroy(m_render_window.value());
	}

	void D3D12RenderSystem::Init(std::optional<Window*> window)
	{
		m_window = window;
		m_device = d3d12::CreateDevice();
		m_direct_queue = d3d12::CreateCommandQueue(m_device, CmdListType::CMD_LIST_DIRECT);

		if (window.has_value())
		{
			m_render_window = d3d12::CreateRenderWindow(m_device, window.value()->GetWindowHandle(), m_direct_queue, d3d12::settings::num_back_buffers);
		}

		PrepareShaderRegistry();
		PrepareRootSignatureRegistry();
		PreparePipelineRegistry();

		LOG("Num Root Signatures: {}", RootSignatureRegistry::Get().m_objects.size());
		LOG("Num Shaders: {}", ShaderRegistry::Get().m_objects.size());
		LOG("Num Pipelines: {}", PipelineRegistry::Get().m_objects.size());

		// Create fences
		for (auto i = 0; i < m_fences.size(); i++)
		{
			m_fences[i] = d3d12::CreateFence(m_device);
		}

		// Temporary
		// Create Constant Buffer Heap
		constexpr auto model_cbs_size = SizeAlign(sizeof(temp::ObjectData) * d3d12::settings::num_instances_per_batch, 256) * d3d12::settings::num_back_buffers;
		constexpr auto cam_cbs_size = SizeAlign(sizeof(temp::ProjectionView_CBData), 256) * d3d12::settings::num_back_buffers;
		constexpr auto sbo_size = 
			(model_cbs_size * 2) /* TODO: Make this more dynamic; right now it only supports 2 mesh nodes */ 
			+ cam_cbs_size;

		m_cb_heap = d3d12::CreateHeap_SBO(m_device, sbo_size, ResourceType::BUFFER, d3d12::settings::num_back_buffers);

		// Create Constant Buffer
		d3d12::MapHeap(m_cb_heap);

		// Load Shaders.
		m_vertex_shader = d3d12::LoadShader(ShaderType::VERTEX_SHADER, "basic.hlsl", "main_vs");
		m_pixel_shader = d3d12::LoadShader(ShaderType::PIXEL_SHADER, "basic.hlsl", "main_ps");

		// Create Root Signature
		d3d12::desc::RootSignatureDesc rs_desc;
		rs_desc.m_samplers.push_back({ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_MIRROR });
		rs_desc.m_parameters.resize(2);
		rs_desc.m_parameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
		rs_desc.m_parameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);

		m_root_signature = d3d12::CreateRootSignature(rs_desc);
		d3d12::FinalizeRootSignature(m_root_signature, m_device);

		// Create Pipeline State
		d3d12::desc::PipelineStateDesc pso_desc;
		pso_desc.m_dsv_format = Format::UNKNOWN;
		pso_desc.m_num_rtv_formats = 1;
		pso_desc.m_rtv_formats[0] = Format::R8G8B8A8_UNORM;
		pso_desc.m_input_layout = wr::Vertex::GetInputLayout();

		m_pipeline_state = d3d12::CreatePipelineState();
		d3d12::SetVertexShader(m_pipeline_state, m_vertex_shader);
		d3d12::SetFragmentShader(m_pipeline_state, m_pixel_shader);
		d3d12::SetRootSignature(m_pipeline_state, m_root_signature);
		d3d12::FinalizePipeline(m_pipeline_state, m_device, pso_desc);

		// Create viewport
		m_viewport = d3d12::CreateViewport(window.has_value() ? window.value()->GetWidth() : 400, window.has_value() ? window.value()->GetHeight() : 400);

		// Create screen quad
		m_vertex_buffer = d3d12::CreateStagingBuffer(m_device, (void*)temp::quad_vertices, 4 * sizeof(Vertex), sizeof(Vertex), ResourceState::VERTEX_AND_CONSTANT_BUFFER);

		// Create Command List
		m_direct_cmd_list = d3d12::CreateCommandList(m_device, d3d12::settings::num_back_buffers, CmdListType::CMD_LIST_DIRECT);

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

	std::unique_ptr<Texture> D3D12RenderSystem::Render(std::shared_ptr<SceneGraph> const & scene_graph, FrameGraph & frame_graph)
	{
		using recursive_func_t = std::function<void(std::shared_ptr<Node>)>;
		recursive_func_t recursive_update = [this, &recursive_update](std::shared_ptr<Node> const & parent)
		{
			for (auto & node : parent->m_children)
			{
				node->Update(this, node.get());
				recursive_update(node);
			}
		};

		recursive_update(scene_graph->GetRootNode());

		frame_graph.Execute(*this, *scene_graph.get());

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
		return std::make_shared<D3D12ModelPool>(*this, size_in_mb);
	}

	void D3D12RenderSystem::PreparePipelineRegistry()
	{
		auto& registry = PipelineRegistry::Get();


		for (auto desc : registry.m_descriptions)
		{
			d3d12::desc::PipelineStateDesc n_desc;
			n_desc.m_counter_clockwise = desc.second.m_counter_clockwise;
			n_desc.m_cull_mode = desc.second.m_cull_mode;
			n_desc.m_depth_enabled = desc.second.m_depth_enabled;
			n_desc.m_dsv_format = desc.second.m_dsv_format;
			n_desc.m_input_layout = desc.second.m_input_layout;
			n_desc.m_num_rtv_formats = desc.second.m_num_rtv_formats;
			n_desc.m_rtv_formats = desc.second.m_rtv_formats;
			n_desc.m_topology_type = desc.second.m_topology_type;
			n_desc.m_type = desc.second.m_type;

			auto n_pipeline = d3d12::CreatePipelineState();

			if (desc.second.m_vertex_shader_handle.has_value())
			{
				auto obj = ShaderRegistry::Get().Find(desc.second.m_vertex_shader_handle.value());
				d3d12::SetVertexShader(n_pipeline, static_cast<D3D12Shader*>(obj)->m_native);
			}
			if (desc.second.m_pixel_shader_handle.has_value())
			{
				auto obj = ShaderRegistry::Get().Find(desc.second.m_pixel_shader_handle.value());
				d3d12::SetFragmentShader(n_pipeline, static_cast<D3D12Shader*>(obj)->m_native);
			}
			if (desc.second.m_compute_shader_handle.has_value())
			{
				auto obj = ShaderRegistry::Get().Find(desc.second.m_compute_shader_handle.value());
				d3d12::SetComputeShader(n_pipeline, static_cast<D3D12Shader*>(obj)->m_native);
			}
			{
				auto obj = RootSignatureRegistry::Get().Find(desc.second.m_root_signature_handle);
				d3d12::SetRootSignature(n_pipeline, static_cast<D3D12RootSignature*>(obj)->m_native);
			}

			d3d12::FinalizePipeline(n_pipeline, m_device, n_desc);

			D3D12Pipeline* pipeline = new D3D12Pipeline();
			pipeline->m_native = n_pipeline;

			registry.m_objects.insert({ desc.first, pipeline });
		}
	}

	void D3D12RenderSystem::PrepareRootSignatureRegistry()
	{
		auto& registry = RootSignatureRegistry::Get();

		for (auto desc : registry.m_descriptions)
		{
			auto rs = new D3D12RootSignature();
			d3d12::desc::RootSignatureDesc n_desc;
			n_desc.m_parameters = desc.second.m_parameters;
			n_desc.m_samplers = desc.second.m_samplers;

			auto n_rs = d3d12::CreateRootSignature(n_desc);
			d3d12::FinalizeRootSignature(n_rs, m_device);
			rs->m_native = n_rs;

			registry.m_objects.insert({ desc.first, rs });
		}
	}

	void D3D12RenderSystem::PrepareShaderRegistry()
	{
		auto& registry = ShaderRegistry::Get();

		for (auto desc : registry.m_descriptions)
		{
			auto shader = new D3D12Shader();
			auto n_shader = d3d12::LoadShader(desc.second.type, desc.second.path, desc.second.entry);
			shader->m_native = n_shader;

			registry.m_objects.insert({ desc.first, shader });
		}
	}

	void D3D12RenderSystem::InitSceneGraph(SceneGraph& scene_graph)
	{
		auto frame_idx = GetFrameIdx();
		d3d12::WaitFor(m_fences[frame_idx]);
		d3d12::Begin(m_direct_cmd_list, frame_idx);


		using recursive_func_t = std::function<void(std::shared_ptr<Node>)>;
		recursive_func_t recursive_init = [this, &recursive_init](std::shared_ptr<Node> const & parent)
		{
			for (auto & node : parent->m_children)
			{
				node->Init(this, node.get());
				recursive_init(node);
			}
		};
		recursive_init(scene_graph.GetRootNode());

		d3d12::End(m_direct_cmd_list);

		// Execute
		d3d12::Execute(m_direct_queue, { m_direct_cmd_list }, m_fences[frame_idx]);
		m_fences[frame_idx]->m_fence_value++;
		d3d12::Signal(m_fences[frame_idx], m_direct_queue);
	}

	void D3D12RenderSystem::RenderSceneGraph(SceneGraph& scene_graph)
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

		recursive_render(scene_graph.GetRootNode());
		RenderMeshBatches(scene_graph);

	}

	void D3D12RenderSystem::Init_MeshNode(MeshNode* node)
	{
		for (auto& mesh : node->m_model->m_meshes)
		{
			auto n_mesh = static_cast<D3D12Mesh*>(mesh);
			d3d12::StageBuffer(n_mesh->m_vertex_buffer, m_direct_cmd_list);
		}
	}

	void wr::D3D12RenderSystem::Init_CameraNode(CameraNode * node)
	{
		auto camera_cb = new D3D12ConstantBufferHandle();
		camera_cb->m_native = d3d12::AllocConstantBuffer(m_cb_heap, sizeof(temp::ProjectionView_CBData));
		node->m_camera_cb = camera_cb;
	}

	void wr::D3D12RenderSystem::Update_MeshNode(MeshNode * node)
	{
		if (!node->RequiresUpdate(GetFrameIdx())) return;

		node->UpdateTemp(GetFrameIdx());

	}

	void wr::D3D12RenderSystem::Update_CameraNode(CameraNode * node)
	{
		if (!node->RequiresUpdate(GetFrameIdx())) return;

		node->UpdateTemp(GetFrameIdx());

		temp::ProjectionView_CBData data;
		data.m_projection = node->m_projection;
		data.m_view = node->m_view;

		auto d3d12_cb_handle = static_cast<D3D12ConstantBufferHandle*>(node->m_camera_cb);

		d3d12::UpdateConstantBuffer(d3d12_cb_handle->m_native, GetFrameIdx(), &data, sizeof(temp::ProjectionView_CBData));
	}

	void D3D12RenderSystem::Render_MeshNode(MeshNode* node)
	{
	}

	//Render batches
	void D3D12RenderSystem::RenderMeshBatches(SceneGraph& scene_graph)
	{
		
		auto& batches = scene_graph.GetBatches();

		//Update if it isn't already

		bool should_update = batches.size() == 0;

		for(auto& elem : batches)
			if (elem.second.num_instances == 0) {
				should_update = true;
				break;
			}

		if(should_update)
			scene_graph.Optimize();

		//Render batches
		for (auto &elem : scene_graph.GetBatches()) {

			Model* model = elem.first;
			temp::MeshBatch& batch = elem.second;

			//Bind object data
			auto d3d12_cb_handle = static_cast<D3D12ConstantBufferHandle*>(batch.batchBuffer);
			d3d12::BindConstantBuffer(m_direct_cmd_list, d3d12_cb_handle->m_native, 1, GetFrameIdx());

			//Render meshes
			for (auto& mesh : model->m_meshes)
			{
				auto n_mesh = static_cast<D3D12Mesh*>(mesh);
				d3d12::BindVertexBuffer(m_direct_cmd_list, n_mesh->m_vertex_buffer);

				//TODO: Don't hardcode the vertices; and support indices
				d3d12::Draw(m_direct_cmd_list, 4, batch.num_instances);
			}

			//Reset instances
			batch.num_instances = 0;
		}
	}

	void wr::D3D12RenderSystem::Render_CameraNode(CameraNode * node)
	{
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
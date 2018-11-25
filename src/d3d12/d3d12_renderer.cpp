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
		m_compute_queue = d3d12::CreateCommandQueue(m_device, CmdListType::CMD_LIST_COMPUTE);
		m_copy_queue = d3d12::CreateCommandQueue(m_device, CmdListType::CMD_LIST_COPY);

		if (window.has_value())
		{
			m_render_window = d3d12::CreateRenderWindow(m_device, window.value()->GetWindowHandle(), m_direct_queue, d3d12::settings::num_back_buffers);
		}

		PrepareShaderRegistry();
		PrepareRootSignatureRegistry();
		PreparePipelineRegistry();

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
			(model_cbs_size * 2) /* TODO: Make this more dynamic; right now it only supports 2 models */ 
			+ cam_cbs_size;

		m_cb_heap = d3d12::CreateHeap_SBO(m_device, sbo_size, ResourceType::BUFFER, d3d12::settings::num_back_buffers);

		// Create Constant Buffer
		d3d12::MapHeap(m_cb_heap);

		// Create viewport
		m_viewport = d3d12::CreateViewport(window.has_value() ? window.value()->GetWidth() : 400, window.has_value() ? window.value()->GetHeight() : 400);

		// Create screen quad
		m_vertex_buffer = d3d12::CreateStagingBuffer(m_device, (void*)temp::quad_vertices, sizeof(temp::quad_vertices), sizeof(Vertex2D), ResourceState::VERTEX_AND_CONSTANT_BUFFER);

		// Create Command List
		m_direct_cmd_list = d3d12::CreateCommandList(m_device, d3d12::settings::num_back_buffers, CmdListType::CMD_LIST_DIRECT);

		if (m_render_window.has_value())
		{
			Resize(m_render_window.value()->m_width, m_render_window.value()->m_height);
		}

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
		auto frame_idx = GetFrameIdx();
		d3d12::WaitFor(m_fences[frame_idx]);

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

		auto cmd_lists = frame_graph.GetAllCommandLists<D3D12CommandList>();
		std::vector<d3d12::CommandList*> n_cmd_lists;
		for (auto& list : cmd_lists)
		{
			n_cmd_lists.push_back(list);
		}

		d3d12::Execute(m_direct_queue, n_cmd_lists, m_fences[frame_idx]);
		d3d12::Signal(m_fences[frame_idx], m_direct_queue);

		if (m_render_window.has_value())
		{
			d3d12::Present(m_render_window.value(), m_device);
		}

		return std::unique_ptr<Texture>();
	}

	void D3D12RenderSystem::Resize(std::int32_t width, std::int32_t height)
	{
		if (m_gbuffer != nullptr)
		{
			delete m_gbuffer;
		}

		//Create Render Target
		m_gbuffer = d3d12::CreateRenderTarget(m_device, (uint32_t)width, (uint32_t)height, { true, Format::D32_FLOAT, { Format::R16G16B16A16_FLOAT, Format::R8G8B8A8_SNORM }, 2, { 0, 0, 0, 0 } });

	}

	std::shared_ptr<MaterialPool> D3D12RenderSystem::CreateMaterialPool(std::size_t size_in_mb)
	{
		return std::make_shared<D3D12MaterialPool>(size_in_mb);
	}

	std::shared_ptr<ModelPool> D3D12RenderSystem::CreateModelPool(std::size_t size_in_mb)
	{
		return std::make_shared<D3D12ModelPool>(*this, size_in_mb);
	}

	void D3D12RenderSystem::WaitForAllPreviousWork()
	{
		for (auto& fence : m_fences)
		{
			d3d12::WaitFor(fence);
		}
	}

	CommandList* D3D12RenderSystem::GetDirectCommandList(unsigned int num_allocators)
	{
		return (D3D12CommandList*)d3d12::CreateCommandList(m_device, num_allocators, CmdListType::CMD_LIST_DIRECT);
	}

	wr::CommandList * D3D12RenderSystem::GetBundleCommandList(unsigned int num_allocators)
	{
		return (D3D12CommandList*)d3d12::CreateCommandList(m_device, num_allocators, CmdListType::CMD_LIST_BUNDLE);
	}

	CommandList* D3D12RenderSystem::GetComputeCommandList(unsigned int num_allocators)
	{
		return (D3D12CommandList*)d3d12::CreateCommandList(m_device, num_allocators, CmdListType::CMD_LIST_DIRECT);
	}

	CommandList* D3D12RenderSystem::GetCopyCommandList(unsigned int num_allocators)
	{
		return (D3D12CommandList*)d3d12::CreateCommandList(m_device, num_allocators, CmdListType::CMD_LIST_DIRECT);
	}

	RenderTarget* D3D12RenderSystem::GetRenderTarget(RenderTargetProperties properties)
	{
		if (properties.m_is_render_window)
		{
			if (!m_render_window.has_value())
			{
				LOGC("Tried using a render task which depends on the render window.");
				return nullptr;
			}
			return (D3D12RenderTarget*)m_render_window.value();
		}
		else
		{
			d3d12::desc::RenderTargetDesc desc;
			desc.m_create_dsv_buffer = properties.m_create_dsv_buffer;
			desc.m_num_rtv_formats = properties.m_num_rtv_formats;
			desc.m_rtv_formats = properties.m_rtv_formats;
			desc.m_dsv_format = properties.m_dsv_format;

			if (!properties.width.has_value() || !properties.height.has_value())
			{
				LOGC("Render target is not the render window and is missing dimensions.");
			}

			return (D3D12RenderTarget*)d3d12::CreateRenderTarget(m_device, properties.width.value(), properties.height.value(), desc, false);
		}
	}

	void D3D12RenderSystem::StartRenderTask(CommandList* cmd_list, std::pair<RenderTarget*, RenderTargetProperties> render_target)
	{
		auto n_cmd_list = static_cast<D3D12CommandList*>(cmd_list);
		auto n_render_target = static_cast<D3D12RenderTarget*>(render_target.first);
		auto frame_idx = GetFrameIdx();
	
		d3d12::Begin(n_cmd_list, frame_idx);
		d3d12::Transition(n_cmd_list, n_render_target, frame_idx, ResourceState::PRESENT, ResourceState::RENDER_TARGET);
		d3d12::BindRenderTargetVersioned(n_cmd_list, n_render_target, frame_idx, render_target.second.m_clear, render_target.second.m_clear_depth);
	}

	void D3D12RenderSystem::StopRenderTask(CommandList* cmd_list, std::pair<RenderTarget*, RenderTargetProperties> render_target)
	{
		auto n_cmd_list = static_cast<D3D12CommandList*>(cmd_list);
		auto n_render_target = static_cast<D3D12RenderTarget*>(render_target.first);
		auto frame_idx = GetFrameIdx();

		d3d12::Transition(n_cmd_list, n_render_target, frame_idx, ResourceState::RENDER_TARGET, ResourceState::PRESENT);
		d3d12::End(n_cmd_list);
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
			n_desc.m_descriptorRanges = desc.second.m_descriptorRanges;

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

	void D3D12RenderSystem::RenderSceneGraph(SceneGraph& scene_graph, D3D12CommandList* cmd_list)
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
		RenderMeshBatches(scene_graph, cmd_list);

	}

	void D3D12RenderSystem::Init_MeshNode(MeshNode* node)
	{
		for (auto& mesh : node->m_model->m_meshes)
		{
			auto n_mesh = static_cast<D3D12Mesh*>(mesh);
			d3d12::StageBuffer(n_mesh->m_vertex_buffer, m_direct_cmd_list);

			if (n_mesh->m_index_buffer != nullptr)
			{
				d3d12::StageBuffer(n_mesh->m_index_buffer, m_direct_cmd_list);
			}
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
	void D3D12RenderSystem::RenderMeshBatches(SceneGraph& scene_graph, D3D12CommandList* cmd_list)
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
			d3d12::BindConstantBuffer(cmd_list, d3d12_cb_handle->m_native, 1, GetFrameIdx());

			//Render meshes
			for (auto& mesh : model->m_meshes)
			{
				auto n_mesh = static_cast<D3D12Mesh*>(mesh);
				d3d12::BindVertexBuffer(cmd_list, n_mesh->m_vertex_buffer);

				if (n_mesh->m_index_buffer != nullptr) 
				{
					d3d12::BindIndexBuffer(cmd_list, n_mesh->m_index_buffer);
					d3d12::DrawIndexed(cmd_list, n_mesh->m_index_count, batch.num_instances);
				}
				else 
				{
					d3d12::Draw(cmd_list, n_mesh->m_vertex_count, batch.num_instances);
				}
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
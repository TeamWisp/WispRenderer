#include "d3d12_renderer.hpp"

#include "../util/defines.hpp"
#include "../util/log.hpp"
#include "../scene_graph/scene_graph.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../window.hpp"

#include "d3d12_defines.hpp"
#include "../material_pool.hpp"
#include "d3d12_resource_pool_texture.hpp"
#include "d3d12_model_pool.hpp"
#include "d3d12_constant_buffer_pool.hpp"
#include "d3d12_structured_buffer_pool.hpp"
#include "d3d12_functions.hpp"
#include "d3d12_pipeline_registry.hpp"
#include "d3d12_rt_pipeline_registry.hpp"
#include "d3d12_shader_registry.hpp"
#include "d3d12_root_signature_registry.hpp"
#include "d3d12_resource_pool_texture.hpp"
#include "d3d12_dynamic_descriptor_heap.hpp"

#include "../scene_graph/mesh_node.hpp"
#include "../scene_graph/camera_node.hpp"
#include "../scene_graph/light_node.hpp"

namespace wr
{
	LINK_SG_RENDER_MESHES(D3D12RenderSystem, Render_MeshNodes)
	LINK_SG_INIT_MESHES(D3D12RenderSystem, Init_MeshNodes)
	LINK_SG_INIT_CAMERAS(D3D12RenderSystem, Init_CameraNodes)
	LINK_SG_INIT_LIGHTS(D3D12RenderSystem, Init_LightNodes)
	LINK_SG_UPDATE_MESHES(D3D12RenderSystem, Update_MeshNodes)
	LINK_SG_UPDATE_CAMERAS(D3D12RenderSystem, Update_CameraNodes)
	LINK_SG_UPDATE_LIGHTS(D3D12RenderSystem, Update_LightNodes)
	LINK_SG_UPDATE_TRANSFORMS(D3D12RenderSystem, Update_Transforms)

	D3D12RenderSystem::~D3D12RenderSystem()
	{
		for (int i = 0; i < m_structured_buffer_pools.size(); ++i)
		{
			m_structured_buffer_pools[i].reset();
		}
		
		for (int i = 0; i < m_model_pools.size(); ++i)
		{
			m_model_pools[i].reset();
		}

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
		SetName(m_device, L"Default D3D12 Device");
		m_direct_queue = d3d12::CreateCommandQueue(m_device, CmdListType::CMD_LIST_DIRECT);
		m_compute_queue = d3d12::CreateCommandQueue(m_device, CmdListType::CMD_LIST_COMPUTE);
		m_copy_queue = d3d12::CreateCommandQueue(m_device, CmdListType::CMD_LIST_COPY);
		SetName(m_direct_queue, L"Default D3D12 Direct Command Queue");
		SetName(m_compute_queue, L"Default D3D12 Compute Command Queue");
		SetName(m_copy_queue, L"Default D3D12 Copy Command Queue");

		if (window.has_value())
		{
			m_render_window = d3d12::CreateRenderWindow(m_device, window.value()->GetWindowHandle(), m_direct_queue, d3d12::settings::num_back_buffers);
		}

		PrepareShaderRegistry();
		PrepareRootSignatureRegistry();
		PreparePipelineRegistry();
		PrepareRTPipelineRegistry();

		// Create fences
		for (auto i = 0; i < m_fences.size(); i++)
		{
			m_fences[i] = d3d12::CreateFence(m_device);
			SetName(m_fences[i], (L"Fence " + std::to_wstring(i)));
		}

		// Create viewport
		m_viewport = d3d12::CreateViewport(window.has_value() ? window.value()->GetWidth() : 400, window.has_value() ? window.value()->GetHeight() : 400);

		// Create screen quad
		m_fullscreen_quad_vb = d3d12::CreateStagingBuffer(m_device, (void*)temp::quad_vertices, 4 * sizeof(Vertex2D), sizeof(Vertex2D), ResourceState::VERTEX_AND_CONSTANT_BUFFER);
		SetName(m_fullscreen_quad_vb, L"Fullscreen quad vertex buffer");

		// Create Command List
		m_direct_cmd_list = d3d12::CreateCommandList(m_device, d3d12::settings::num_back_buffers, CmdListType::CMD_LIST_DIRECT);
		SetName(m_direct_cmd_list, L"Defauld DX12 Command List");

		// Raytracing cb pool
		m_raytracing_cb_pool = CreateConstantBufferPool(1);

		// Simple Shapes Model Pool
		m_shapes_pool = CreateModelPool(8, 8);
		LoadPrimitiveShapes();

		// Material raytracing sb pool
		size_t rt_mat_align_size = (sizeof(temp::RayTracingMaterial_CBData) * d3d12::settings::num_max_rt_materials) * d3d12::settings::num_back_buffers;
		m_raytracing_material_sb_pool = CreateStructuredBufferPool(1);

		// Begin Recording
		auto frame_idx = m_render_window.has_value() ? m_render_window.value()->m_frame_idx : 0;
		d3d12::Begin(m_direct_cmd_list, frame_idx);

		// Stage fullscreen quad
		d3d12::StageBuffer(m_fullscreen_quad_vb, m_direct_cmd_list);

		// Execute Indirect code
		if (d3d12::settings::use_exec_indirect)
		{
			m_indirect_cmd_buffer = d3d12::CreateIndirectCommandBuffer(m_device, d3d12::settings::num_indirect_draw_commands, sizeof(temp::IndirectCommand), d3d12::settings::num_back_buffers);
			SetName(m_indirect_cmd_buffer, L"Default indirect command buffer");
			m_indirect_cmd_buffer_indexed = d3d12::CreateIndirectCommandBuffer(m_device, d3d12::settings::num_indirect_index_commands, sizeof(temp::IndirectCommandIndexed), d3d12::settings::num_back_buffers);
			SetName(m_indirect_cmd_buffer_indexed, L"Default indirect command buffer indexed");

			std::vector<D3D12_INDIRECT_ARGUMENT_DESC> arg_descs(4);
			arg_descs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
			arg_descs[0].ConstantBufferView.RootParameterIndex = 0;
			arg_descs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
			arg_descs[1].ConstantBufferView.RootParameterIndex = 1;
			arg_descs[2].Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
			arg_descs[3].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

			std::vector<D3D12_INDIRECT_ARGUMENT_DESC> indexed_arg_descs = arg_descs;
			indexed_arg_descs[3].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

			auto root_signature = static_cast<D3D12RootSignature*>(RootSignatureRegistry::Get().Find(root_signatures::basic));
			m_cmd_signature = d3d12::CreateCommandSignature(m_device, root_signature->m_native, arg_descs, sizeof(temp::IndirectCommand));
			m_cmd_signature_indexed = d3d12::CreateCommandSignature(m_device, root_signature->m_native, indexed_arg_descs, sizeof(temp::IndirectCommandIndexed));
			SetName(m_cmd_signature, L"Defauld DX12 Command Signature");
			SetName(m_cmd_signature_indexed, L"Defauld DX12 Command Signature Indexed");
		}

		// Execute
		d3d12::End(m_direct_cmd_list);
		d3d12::Execute(m_direct_queue, { m_direct_cmd_list }, m_fences[frame_idx]);

		m_buffer_frame_graph_uids.resize(d3d12::settings::num_back_buffers);
	}

	std::unique_ptr<TextureHandle> D3D12RenderSystem::Render(std::shared_ptr<SceneGraph> const & scene_graph, FrameGraph & frame_graph)
	{
		if (m_requested_fullscreen_state.has_value())
		{
			WaitForAllPreviousWork();
			m_render_window.value()->m_swap_chain->SetFullscreenState(m_requested_fullscreen_state.value(), nullptr);
			Resize(m_window.value()->GetWidth(), m_window.value()->GetHeight());
			m_requested_fullscreen_state = std::nullopt;
		}

		auto frame_idx = GetFrameIdx();
		d3d12::WaitFor(m_fences[frame_idx]);
		
		// Perform reload requests
		{
			// Root Signatures
			auto& rt_registry = RootSignatureRegistry::Get();
			for (auto request : rt_registry.m_requested_reload)
			{
				// ReloadPipelineRegistryEntry(request);
			}

			// Shaders
			auto& shader_registry = ShaderRegistry::Get();
			for (auto request : shader_registry.m_requested_reload)
			{
				// ReloadPipelineRegistryEntry(request);
			}

			// Pipelines
			auto& pipeline_registry = PipelineRegistry::Get();
			pipeline_registry.m_reload_request_mutex.lock();
			for (auto request : pipeline_registry.m_requested_reload)
			{
				ReloadPipelineRegistryEntry(request);
			}
			pipeline_registry.m_requested_reload.clear();
			pipeline_registry.m_reload_request_mutex.unlock();

			// RT Pipelines
			auto& rt_pipeline_registry = RTPipelineRegistry::Get();
			for (auto request : rt_pipeline_registry.m_requested_reload)
			{
				ReloadRTPipelineRegistryEntry(request);
			}
		}


		bool clear_frame_buffer = false;

		if (frame_graph.GetUID() != m_buffer_frame_graph_uids[frame_idx])
		{
			m_buffer_frame_graph_uids[frame_idx] = frame_graph.GetUID();
			clear_frame_buffer = true;
		}

		PreparePreRenderCommands(clear_frame_buffer, frame_idx);

		scene_graph->Update();
		scene_graph->Optimize();

		frame_graph.Execute(*this, *scene_graph.get());

		auto cmd_lists = frame_graph.GetAllCommandLists<d3d12::CommandList>();
		std::vector<d3d12::CommandList*> n_cmd_lists;

		n_cmd_lists.push_back(m_direct_cmd_list);

		for (auto& list : cmd_lists)
		{
			n_cmd_lists.push_back(list);
		}

		d3d12::Execute(m_direct_queue, n_cmd_lists, m_fences[frame_idx]);

		if (m_render_window.has_value())
		{
			d3d12::Present(m_render_window.value(), m_device);
		}

		m_bound_model_pool = nullptr;

		//Signal end of frame to the texture pool so that stale descriptors can be freed.
		m_texture_pool->EndOfFrame();

		return std::unique_ptr<TextureHandle>();
	}

	void D3D12RenderSystem::Resize(std::uint32_t width, std::uint32_t height)
	{

		d3d12::ResizeViewport(m_viewport, (int)width, (int)height);
		
		if (m_render_window.has_value())
		{
			d3d12::Resize(m_render_window.value(), m_device, width, height, m_window.value()->IsFullscreen());
		}
	}

	std::shared_ptr<TexturePool> D3D12RenderSystem::CreateTexturePool(std::size_t size_in_mb, std::size_t num_of_textures)
	{
		m_texture_pool = std::make_shared<D3D12TexturePool>(*this, size_in_mb, num_of_textures);
		return m_texture_pool;
	}

	std::shared_ptr<MaterialPool> D3D12RenderSystem::CreateMaterialPool(std::size_t size_in_mb)
	{
		return std::make_shared<MaterialPool>();
	}

	std::shared_ptr<ModelPool> D3D12RenderSystem::CreateModelPool(std::size_t vertex_buffer_pool_size_in_mb, std::size_t index_buffer_pool_size_in_mb)
	{
		std::shared_ptr<D3D12ModelPool> pool = std::make_shared<D3D12ModelPool>(*this, vertex_buffer_pool_size_in_mb, index_buffer_pool_size_in_mb);
		m_model_pools.push_back(pool);
		return pool;
	}

	std::shared_ptr<ConstantBufferPool> D3D12RenderSystem::CreateConstantBufferPool(std::size_t size_in_mb)
	{
		return std::make_shared<D3D12ConstantBufferPool>(*this, size_in_mb);
	}

	std::shared_ptr<StructuredBufferPool> D3D12RenderSystem::CreateStructuredBufferPool(std::size_t size_in_mb)
	{
		std::shared_ptr<D3D12StructuredBufferPool> pool = std::make_shared<D3D12StructuredBufferPool>(*this, size_in_mb); 
		m_structured_buffer_pools.push_back(pool);
		return pool;
	}

	void D3D12RenderSystem::WaitForAllPreviousWork()
	{
		for (auto& fence : m_fences)
		{
			d3d12::WaitFor(fence);
			Signal(fence, m_direct_queue);
		}
	}

	CommandList* D3D12RenderSystem::GetDirectCommandList(unsigned int num_allocators)
	{
		return d3d12::CreateCommandList(m_device, num_allocators, CmdListType::CMD_LIST_DIRECT);
	}

	CommandList* D3D12RenderSystem::GetBundleCommandList(unsigned int num_allocators)
	{
		return d3d12::CreateCommandList(m_device, num_allocators, CmdListType::CMD_LIST_BUNDLE);
	}

	CommandList* D3D12RenderSystem::GetComputeCommandList(unsigned int num_allocators)
	{
		return d3d12::CreateCommandList(m_device, num_allocators, CmdListType::CMD_LIST_DIRECT);
	}

	CommandList* D3D12RenderSystem::GetCopyCommandList(unsigned int num_allocators)
	{
		return d3d12::CreateCommandList(m_device, num_allocators, CmdListType::CMD_LIST_DIRECT);
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
			return m_render_window.value();
		}
		else
		{
			d3d12::desc::RenderTargetDesc desc;
			desc.m_initial_state = properties.m_state_finished.value_or(ResourceState::RENDER_TARGET);
			desc.m_create_dsv_buffer = properties.m_create_dsv_buffer;
			desc.m_num_rtv_formats = properties.m_num_rtv_formats;
			desc.m_rtv_formats = properties.m_rtv_formats;
			desc.m_dsv_format = properties.m_dsv_format;

			if (properties.m_width.has_value() || properties.m_height.has_value())
			{
				auto retval = d3d12::CreateRenderTarget(m_device, properties.m_width.value(), properties.m_height.value(), desc);
				for (auto i = 0; i < retval->m_render_targets.size(); i++)
					retval->m_render_targets[i]->SetName(L"Main Deferred RT");
				return retval;
			}
			else if (m_window.has_value())
			{
				auto retval = d3d12::CreateRenderTarget(m_device, m_window.value()->GetWidth(), m_window.value()->GetHeight(), desc);
				for (auto i = 0; i < retval->m_render_targets.size(); i++)
					retval->m_render_targets[i]->SetName(L"Main Deferred RT");
				return retval;
			}
			else
			{
				LOGC("Render target doesn't have a width or height specified. And there is no window to take the window size from. Hence can't create a proper render target.");
				return nullptr;
			}
		}
	}

	void D3D12RenderSystem::ResizeRenderTarget(RenderTarget** render_target, std::uint32_t width, std::uint32_t height)
	{
		auto n_render_target = static_cast<d3d12::RenderTarget*>(*render_target);
		d3d12::Resize((d3d12::RenderTarget**)&n_render_target, m_device, width, height);

		(*render_target) = n_render_target;
	}

	void D3D12RenderSystem::RequestFullscreenChange(bool fullscreen_state)
	{
		m_requested_fullscreen_state = fullscreen_state;
	}

	void D3D12RenderSystem::StartRenderTask(CommandList* cmd_list, std::pair<RenderTarget*, RenderTargetProperties> render_target)
	{
		auto n_cmd_list = static_cast<d3d12::CommandList*>(cmd_list);
		auto n_render_target = static_cast<d3d12::RenderTarget*>(render_target.first);
		auto frame_idx = GetFrameIdx();
	
		d3d12::Begin(n_cmd_list, frame_idx);

		if (render_target.second.m_is_render_window) // TODO: do once at the beginning of the frame.
		{
			d3d12::Transition(n_cmd_list, n_render_target, frame_idx, ResourceState::PRESENT, ResourceState::RENDER_TARGET);
		}
		else if (render_target.second.m_state_finished.has_value() && render_target.second.m_state_execute.has_value())
		{
			d3d12::Transition(n_cmd_list, n_render_target, render_target.second.m_state_finished.value(), render_target.second.m_state_execute.value());
		}
		else
		{
			LOGW("A render target has no transitions specified. Is this correct?");
		}

		if (render_target.second.m_is_render_window)
		{
			d3d12::BindRenderTargetVersioned(n_cmd_list, n_render_target, frame_idx, render_target.second.m_clear, render_target.second.m_clear_depth);
		}
		else
		{
			d3d12::BindRenderTarget(n_cmd_list, n_render_target, render_target.second.m_clear, render_target.second.m_clear_depth);
		}
	}

	void D3D12RenderSystem::StopRenderTask(CommandList* cmd_list, std::pair<RenderTarget*, RenderTargetProperties> render_target)
	{
		auto n_cmd_list = static_cast<d3d12::CommandList*>(cmd_list);
		auto n_render_target = static_cast<d3d12::RenderTarget*>(render_target.first);
		unsigned int frame_idx = GetFrameIdx();

		if (render_target.second.m_is_render_window)
		{
			d3d12::Transition(n_cmd_list, n_render_target, frame_idx, ResourceState::RENDER_TARGET, ResourceState::PRESENT);
		}
		else if (render_target.second.m_state_finished.has_value() && render_target.second.m_state_execute.has_value())
		{
			d3d12::Transition(n_cmd_list, n_render_target, render_target.second.m_state_execute.value(), render_target.second.m_state_finished.value());
		}
		else
		{
			LOGW("A render target has no transitions specified. Is this correct?");
		}

		d3d12::End(n_cmd_list);
	}

	void D3D12RenderSystem::StartComputeTask(CommandList * cmd_list, std::pair<RenderTarget*, RenderTargetProperties> render_target)
	{
		auto n_cmd_list = static_cast<d3d12::CommandList*>(cmd_list);
		auto n_render_target = static_cast<d3d12::RenderTarget*>(render_target.first);
		auto frame_idx = GetFrameIdx();

		d3d12::Begin(n_cmd_list, frame_idx);
	}

	void D3D12RenderSystem::StopComputeTask(CommandList * cmd_list, std::pair<RenderTarget*, RenderTargetProperties> render_target)
	{
		auto n_cmd_list = static_cast<d3d12::CommandList*>(cmd_list);
		auto n_render_target = static_cast<d3d12::RenderTarget*>(render_target.first);
		auto frame_idx = GetFrameIdx();

		d3d12::End(n_cmd_list);
	}

	void D3D12RenderSystem::StartCopyTask(CommandList * cmd_list, std::pair<RenderTarget*, RenderTargetProperties> render_target)
	{
		auto n_cmd_list = static_cast<d3d12::CommandList*>(cmd_list);
		auto n_render_target = static_cast<d3d12::RenderTarget*>(render_target.first);
		auto frame_idx = GetFrameIdx();

		d3d12::Begin(n_cmd_list, frame_idx);

		if (render_target.second.m_is_render_window) // TODO: do once at the beginning of the frame.
		{
			d3d12::Transition(n_cmd_list, n_render_target, frame_idx, ResourceState::PRESENT, render_target.second.m_state_execute.value());
		}
		else if (render_target.second.m_state_finished.has_value() && render_target.second.m_state_execute.has_value())
		{
			d3d12::Transition(n_cmd_list, n_render_target, render_target.second.m_state_finished.value(), render_target.second.m_state_execute.value());
		}
	}

	void D3D12RenderSystem::StopCopyTask(CommandList * cmd_list, std::pair<RenderTarget*, RenderTargetProperties> render_target)
	{
		auto n_cmd_list = static_cast<d3d12::CommandList*>(cmd_list);
		auto n_render_target = static_cast<d3d12::RenderTarget*>(render_target.first);
		auto frame_idx = GetFrameIdx();

		if (render_target.second.m_is_render_window)
		{
			d3d12::Transition(n_cmd_list, n_render_target, frame_idx, render_target.second.m_state_execute.value(), ResourceState::PRESENT);
		}
		else if (render_target.second.m_state_finished.has_value() && render_target.second.m_state_execute.has_value())
		{
			d3d12::Transition(n_cmd_list, n_render_target, render_target.second.m_state_execute.value(), render_target.second.m_state_finished.value());
		}

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
			n_desc.m_rtx = desc.second.m_rtx;
			n_desc.m_rt_local = desc.second.m_rtx_local;

			auto n_rs = d3d12::CreateRootSignature(n_desc);
			d3d12::FinalizeRootSignature(n_rs, m_device);
			rs->m_native = n_rs;
			SetName(n_rs, (L"Root Signature " + desc.second.name));

			registry.m_objects.insert({ desc.first, rs });
		}
	}

	void D3D12RenderSystem::PrepareShaderRegistry()
	{
		auto& registry = ShaderRegistry::Get();

		for (auto desc : registry.m_descriptions)
		{
			auto shader = new D3D12Shader();
			auto shader_error = d3d12::LoadShader(desc.second.type, desc.second.path, desc.second.entry);

			if (std::holds_alternative<d3d12::Shader*>(shader_error))
			{
				auto n_shader = std::get<d3d12::Shader*>(shader_error);
				shader->m_native = n_shader;
				registry.m_objects.insert({ desc.first, shader });
			}
			else
			{
				LOGC("Failed to load shader. compiler error: {}", std::get<std::string>(shader_error));
			}
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
				auto& shader = static_cast<D3D12Shader*>(obj)->m_native;
				d3d12::SetVertexShader(n_pipeline, shader);
			}
			if (desc.second.m_pixel_shader_handle.has_value())
			{
				auto obj = ShaderRegistry::Get().Find(desc.second.m_pixel_shader_handle.value());
				auto& shader = static_cast<D3D12Shader*>(obj)->m_native;
				d3d12::SetFragmentShader(n_pipeline, shader);
			}
			if (desc.second.m_compute_shader_handle.has_value())
			{
				auto obj = ShaderRegistry::Get().Find(desc.second.m_compute_shader_handle.value());
				auto& shader = static_cast<D3D12Shader*>(obj)->m_native;
				d3d12::SetComputeShader(n_pipeline, shader);
			}
			{
				auto obj = RootSignatureRegistry::Get().Find(desc.second.m_root_signature_handle);
				d3d12::SetRootSignature(n_pipeline, static_cast<D3D12RootSignature*>(obj)->m_native);
			}

			d3d12::FinalizePipeline(n_pipeline, m_device, n_desc);

			D3D12Pipeline* pipeline = new D3D12Pipeline();
			pipeline->m_native = n_pipeline;
			SetName(n_pipeline, L"Default pipeline state");

			registry.m_objects.insert({ desc.first, pipeline });
		}
	}

	void D3D12RenderSystem::ReloadPipelineRegistryEntry(RegistryHandle handle)
	{
		auto& registry = PipelineRegistry::Get();
		std::optional<std::string> error_msg = std::nullopt;
		auto n_pipeline = static_cast<D3D12Pipeline*>(registry.Find(handle))->m_native;

		auto recompile_shader = [&error_msg](auto& pipeline_shader)
		{
			if (!pipeline_shader) return;

			auto new_shader_variant = d3d12::LoadShader(pipeline_shader->m_type,
				pipeline_shader->m_path,
				pipeline_shader->m_entry);

			if (std::holds_alternative<d3d12::Shader*>(new_shader_variant))
			{
				pipeline_shader = std::get<d3d12::Shader*>(new_shader_variant);
			}
			else
			{
				error_msg = std::get<std::string>(new_shader_variant);
			}
		};

		// Vertex Shader
		{
			recompile_shader(n_pipeline->m_vertex_shader);
		}
		// Pixel Shader
		if (!error_msg.has_value()) {
			recompile_shader(n_pipeline->m_pixel_shader);
		}
		// Compute Shader
		if (!error_msg.has_value()) {
			recompile_shader(n_pipeline->m_compute_shader);
		}

		if (error_msg.has_value())
		{
			LOGW(error_msg.value());
			//open_shader_compiler_popup = true;
			//shader_compiler_error = error_msg.value();
		}
		else
		{
			d3d12::RefinalizePipeline(n_pipeline);
		}
	}

	void D3D12RenderSystem::ReloadRTPipelineRegistryEntry(RegistryHandle handle)
	{
		auto& registry = RTPipelineRegistry::Get();
		std::optional<std::string> error_msg = std::nullopt;
		auto n_pipeline = static_cast<D3D12StateObject*>(registry.Find(handle))->m_native;

		auto recompile_shader = [&error_msg](auto& pipeline_shader)
		{
			auto new_shader_variant = d3d12::LoadShader(pipeline_shader->m_type,
				pipeline_shader->m_path,
				pipeline_shader->m_entry);

			if (std::holds_alternative<d3d12::Shader*>(new_shader_variant))
			{
				pipeline_shader = std::get<d3d12::Shader*>(new_shader_variant);
			}
			else
			{
				error_msg = std::get<std::string>(new_shader_variant);
			}
		};

		// Vertex Shader
		{
			recompile_shader(n_pipeline->m_desc.m_library);
		}

		if (error_msg.has_value())
		{
			LOGW(error_msg.value());
			//open_shader_compiler_popup = true;
			//shader_compiler_error = error_msg.value();
		}
		else
		{
			d3d12::RecreateStateObject(n_pipeline);
		}
	}

	void D3D12RenderSystem::PrepareRTPipelineRegistry()
	{
		auto& registry = RTPipelineRegistry::Get();

		for (auto it : registry.m_descriptions)
		{
			auto desc = it.second;
			auto obj = new D3D12StateObject();

			auto library = static_cast<D3D12Shader*>(ShaderRegistry::Get().Find(desc.library_desc.shader_handle));

			d3d12::desc::StateObjectDesc n_desc;
			n_desc.m_library = library->m_native;
			n_desc.m_library_exports = desc.library_desc.exports;
			n_desc.max_attributes_size = desc.max_attributes_size;
			n_desc.max_payload_size = desc.max_payload_size;
			n_desc.max_recursion_depth = desc.max_recursion_depth;
			n_desc.is_hybrid = desc.is_hybrid;
			n_desc.m_hit_groups = desc.library_desc.m_hit_groups;

			if (auto rt_handle = desc.global_root_signature.value(); desc.global_root_signature.has_value())
			{
				auto library = static_cast<D3D12RootSignature*>(RootSignatureRegistry::Get().Find(rt_handle));
				n_desc.global_root_signature = library->m_native;
			}

			if (desc.local_root_signatures.has_value())
			{
				n_desc.local_root_signatures = std::vector<d3d12::RootSignature*>();
				for (auto rt_handle : desc.local_root_signatures.value())
				{
					auto library = static_cast<D3D12RootSignature*>(RootSignatureRegistry::Get().Find(rt_handle));
					n_desc.local_root_signatures.value().push_back(library->m_native);
				}
			}

			obj->m_native = d3d12::CreateStateObject(m_device, n_desc);

			registry.m_objects.insert({ it.first, obj });
		}
	}

	void D3D12RenderSystem::InitSceneGraph(SceneGraph& scene_graph)
	{
		auto frame_idx = GetFrameIdx();
		d3d12::WaitFor(m_fences[frame_idx]);
		d3d12::Begin(m_direct_cmd_list, frame_idx);

		scene_graph.Init();

		d3d12::End(m_direct_cmd_list);

		// Execute
		d3d12::Execute(m_direct_queue, { m_direct_cmd_list }, m_fences[frame_idx]);
	}

	void D3D12RenderSystem::Init_MeshNodes(std::vector<std::shared_ptr<MeshNode>>& nodes)
	{
	}

	void D3D12RenderSystem::Init_CameraNodes(std::vector<std::shared_ptr<CameraNode>>& nodes)
	{
		if (nodes.empty()) return;

		size_t cam_align_size = SizeAlign(nodes.size() * sizeof(temp::ProjectionView_CBData), 256) * d3d12::settings::num_back_buffers;
		m_camera_pool = CreateConstantBufferPool((size_t) std::ceil(cam_align_size));

		for (auto& node : nodes)
		{
			node->m_camera_cb = m_camera_pool->Create(sizeof(temp::ProjectionView_CBData));
		}
	}

	void D3D12RenderSystem::Init_LightNodes(std::vector<std::shared_ptr<LightNode>>& nodes, std::vector<Light>& lights)
	{
	}

	void D3D12RenderSystem::Update_Transforms(SceneGraph& scene_graph, std::shared_ptr<Node>& node)
	{

		if (node->RequiresTransformUpdate(GetFrameIdx()))
		{
			node->UpdateTransform();
			node->SignalTransformUpdate(GetFrameIdx());
		}

		auto& children = node->m_children;
		auto it = children.begin();

		while (it != children.end())
		{
			Update_Transforms(scene_graph, *it);
			++it;
		}

	}

	void D3D12RenderSystem::PreparePreRenderCommands(bool clear_frame_buffer, int frame_idx)
	{
		d3d12::Begin(m_direct_cmd_list, frame_idx);

		if (clear_frame_buffer)
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_descriptor(m_render_window.value()->m_rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart());

			rtv_descriptor.Offset(frame_idx, m_render_window.value()->m_rtv_descriptor_increment_size);

			float clear_color[] = { 0, 0, 0, 0 };

			m_direct_cmd_list->m_native->ResourceBarrier(1,
				&CD3DX12_RESOURCE_BARRIER::Transition(
					m_render_window.value()->m_render_targets[frame_idx],
					D3D12_RESOURCE_STATE_PRESENT,
					D3D12_RESOURCE_STATE_RENDER_TARGET));

			m_direct_cmd_list->m_native->ClearRenderTargetView(
				rtv_descriptor,
				clear_color,
				0,
				nullptr
			);


			m_direct_cmd_list->m_native->ResourceBarrier(1,
				&CD3DX12_RESOURCE_BARRIER::Transition(
					m_render_window.value()->m_render_targets[frame_idx],
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_PRESENT));
		}

		for (int i = 0; i < m_structured_buffer_pools.size(); ++i)
		{
			m_structured_buffer_pools[i]->UpdateBuffers(m_direct_cmd_list, frame_idx);
		}

		for (int i = 0; i < m_model_pools.size(); ++i)
		{
			m_model_pools[i]->StageMeshes(m_direct_cmd_list);
		}

		m_texture_pool->Stage(m_direct_cmd_list);

		d3d12::End(m_direct_cmd_list);
	}

	void D3D12RenderSystem::Update_MeshNodes(std::vector<std::shared_ptr<MeshNode>>& nodes)
	{
		for (auto& node : nodes)
		{
			if (!node->RequiresUpdate(GetFrameIdx()))
			{
				continue;
			}

			node->Update(GetFrameIdx());
		}
	}

	void D3D12RenderSystem::Update_CameraNodes(std::vector<std::shared_ptr<CameraNode>>& nodes)
	{
		for (auto& node : nodes)
		{
			if (!node->RequiresUpdate(GetFrameIdx()))
			{
				continue;
			}

			node->UpdateTemp(GetFrameIdx());

			temp::ProjectionView_CBData data;
			data.m_projection = node->m_projection;
			data.m_inverse_projection = node->m_inverse_projection;
			data.m_view = node->m_view;

			node->m_camera_cb->m_pool->Update(node->m_camera_cb, sizeof(temp::ProjectionView_CBData), 0, (uint8_t*) &data);
		}
	}

	void D3D12RenderSystem::Update_LightNodes(SceneGraph& scene_graph)
	{
		bool should_update = false;
		uint32_t offset_start = 0, offset_end = 0;

		std::vector<std::shared_ptr<LightNode>>& light_nodes = scene_graph.GetLightNodes();

		for (uint32_t i = 0, j = (uint32_t) light_nodes.size(); i < j; ++i)
		{
			std::shared_ptr<LightNode>& node = light_nodes[i];

			if (!node->RequiresUpdate(GetFrameIdx()))
			{
				continue;
			}

			if (!should_update)
			{
				should_update = true;
				offset_start = i;
			}

			node->Update(GetFrameIdx());

			offset_end = i;
		}

		if (!should_update)
			return;

		StructuredBufferHandle* structured_buffer = scene_graph.GetLightBuffer();

		structured_buffer->m_pool->Update(structured_buffer, scene_graph.GetLight(offset_start), sizeof(Light) * (offset_end - offset_start + 1), sizeof(Light) * offset_start);

	}

	void D3D12RenderSystem::Render_MeshNodes(temp::MeshBatches& batches, CameraNode* camera, CommandList* cmd_list)
	{
		auto n_cmd_list = static_cast<d3d12::CommandList*>(cmd_list);
		auto frame_idx = GetFrameIdx();
		auto d3d12_camera_cb = static_cast<D3D12ConstantBufferHandle*>(camera->m_camera_cb);
		std::vector<temp::IndirectCommand> commands;
		std::vector<temp::IndirectCommandIndexed> indexed_commands;

		if constexpr (!d3d12::settings::use_exec_indirect)
		{
			d3d12::BindConstantBuffer(n_cmd_list, d3d12_camera_cb->m_native, 0, GetFrameIdx());
		}

		//Render batches
		for (auto& elem : batches)
		{
			Model* model = elem.first;
			temp::MeshBatch& batch = elem.second;

			// Execute Indirect Pipeline
			if constexpr (d3d12::settings::use_exec_indirect)
			{
				//Render meshes
				for (auto& mesh : model->m_meshes)
				{
					auto n_mesh = static_cast<D3D12ModelPool*>(model->m_model_pool)->GetMeshData(mesh.first->id);
					auto vb = static_cast<D3D12ModelPool*>(model->m_model_pool)->GetVertexStagingBuffer();
					auto ib = static_cast<D3D12ModelPool*>(model->m_model_pool)->GetIndexStagingBuffer();
					auto d3d12_cb_handle = static_cast<D3D12ConstantBufferHandle*>(batch.batch_buffer);

					d3d12::BindIndexBuffer(n_cmd_list,
						ib,
						0,
						ib->m_size);

					if (n_mesh->m_index_staging_buffer_size != 0)
					{
						// temporary
						D3D12_VERTEX_BUFFER_VIEW view;
						view.BufferLocation = vb->m_gpu_address;
						view.StrideInBytes = n_mesh->m_vertex_staging_buffer_stride;
						view.SizeInBytes = vb->m_size;

						temp::IndirectCommandIndexed command;
						command.cbv_camera = d3d12_camera_cb->m_native->m_gpu_addresses[frame_idx];
						command.cbv_object = d3d12_cb_handle->m_native->m_gpu_addresses[frame_idx];
						command.vb_view = view;
						command.draw_arguments.IndexCountPerInstance = n_mesh->m_index_count;
						command.draw_arguments.InstanceCount = batch.num_instances;
						command.draw_arguments.StartIndexLocation = n_mesh->m_index_staging_buffer_offset;
						command.draw_arguments.StartInstanceLocation = 0;
						command.draw_arguments.BaseVertexLocation = n_mesh->m_vertex_staging_buffer_offset; // 1170.sometghing fuck me
						indexed_commands.push_back(command);
					}
					else
					{
						// temporary
						D3D12_VERTEX_BUFFER_VIEW view;
						view.BufferLocation = vb->m_gpu_address;
						view.StrideInBytes = n_mesh->m_vertex_staging_buffer_stride;
						view.SizeInBytes = vb->m_size;

						temp::IndirectCommand command;
						command.cbv_camera = d3d12_camera_cb->m_native->m_gpu_addresses[frame_idx];
						command.cbv_object = d3d12_cb_handle->m_native->m_gpu_addresses[frame_idx];
						command.vb_view = view;
						command.draw_arguments.VertexCountPerInstance = n_mesh->m_vertex_count;
						command.draw_arguments.InstanceCount = batch.num_instances;
						command.draw_arguments.StartVertexLocation = n_mesh->m_vertex_staging_buffer_offset;
						command.draw_arguments.StartInstanceLocation = 0;
						commands.push_back(command);
					}
				}
			}
			// Draw Command Pipeline
			else
			{
				//Bind object data
				auto d3d12_cb_handle = static_cast<D3D12ConstantBufferHandle*>(batch.batch_buffer);
				if constexpr (!d3d12::settings::use_exec_indirect)
				{
					d3d12::BindConstantBuffer(n_cmd_list, d3d12_cb_handle->m_native, 1, GetFrameIdx());
				}

				//Render meshes
				for (auto& mesh : model->m_meshes)
				{
					auto n_mesh = static_cast<D3D12ModelPool*>(model->m_model_pool)->GetMeshData(mesh.first->id);
					if (model->m_model_pool != m_bound_model_pool || n_mesh->m_vertex_staging_buffer_stride != m_bound_model_pool_stride) 
					{
						d3d12::BindVertexBuffer(n_cmd_list,
							static_cast<D3D12ModelPool*>(model->m_model_pool)->GetVertexStagingBuffer(),
							0,
							static_cast<D3D12ModelPool*>(model->m_model_pool)->GetVertexStagingBuffer()->m_size,
							n_mesh->m_vertex_staging_buffer_stride);

						d3d12::BindIndexBuffer(n_cmd_list,
							static_cast<D3D12ModelPool*>(model->m_model_pool)->GetIndexStagingBuffer(),
							0,
							static_cast<D3D12ModelPool*>(model->m_model_pool)->GetIndexStagingBuffer()->m_size);

						m_bound_model_pool = static_cast<D3D12ModelPool*>(model->m_model_pool);
						m_bound_model_pool_stride = n_mesh->m_vertex_staging_buffer_stride;
					}
					
					d3d12::BindDescriptorHeaps(n_cmd_list, frame_idx);

					auto material_handle = mesh.second;
					
					if (material_handle != m_last_material)
					{
						m_last_material = material_handle;

						BindMaterial(material_handle, cmd_list);
					}

					if (n_mesh->m_index_count != 0)
					{
						d3d12::DrawIndexed(n_cmd_list, n_mesh->m_index_count, batch.num_instances, n_mesh->m_index_staging_buffer_offset, n_mesh->m_vertex_staging_buffer_offset);
					}
					else
					{
						d3d12::Draw(n_cmd_list, n_mesh->m_vertex_count, batch.num_instances, n_mesh->m_vertex_staging_buffer_offset);
					}
				}
			}

			//Reset instances
			batch.num_instances = 0;
			m_last_material = nullptr;
		}

		if constexpr (d3d12::settings::use_exec_indirect)
		{
			if (std::size_t size = commands.size(); size > 0)
			{
				d3d12::Transition(n_cmd_list, m_indirect_cmd_buffer, ResourceState::INDIRECT_ARGUMENT, ResourceState::COPY_DEST, frame_idx);
				d3d12::StageBuffer(n_cmd_list, m_indirect_cmd_buffer, commands.data(), size, frame_idx);
				d3d12::Transition(n_cmd_list, m_indirect_cmd_buffer, ResourceState::COPY_DEST, ResourceState::INDIRECT_ARGUMENT, frame_idx);
				d3d12::ExecuteIndirect(n_cmd_list, m_cmd_signature, m_indirect_cmd_buffer, frame_idx);
			}
			if (std::size_t size = indexed_commands.size(); size > 0)
			{
				d3d12::Transition(n_cmd_list, m_indirect_cmd_buffer_indexed, ResourceState::INDIRECT_ARGUMENT, ResourceState::COPY_DEST, frame_idx);
				d3d12::StageBuffer(n_cmd_list, m_indirect_cmd_buffer_indexed, indexed_commands.data(), size, frame_idx);
				d3d12::Transition(n_cmd_list, m_indirect_cmd_buffer_indexed, ResourceState::COPY_DEST, ResourceState::INDIRECT_ARGUMENT, frame_idx);
				d3d12::ExecuteIndirect(n_cmd_list, m_cmd_signature_indexed, m_indirect_cmd_buffer_indexed, frame_idx);
			}
		}
	}

	void D3D12RenderSystem::BindMaterial(MaterialHandle* material_handle, CommandList* cmd_list)
	{
		auto n_cmd_list = static_cast<d3d12::CommandList*>(cmd_list);

		auto* material_internal = material_handle->m_pool->GetMaterial(material_handle->m_id);

		auto albedo_handle = material_internal->GetAlbedo();
		auto* albedo_internal = static_cast<wr::d3d12::TextureResource*>(albedo_handle.m_pool->GetTexture(albedo_handle.m_id));

		auto normal_handle = material_internal->GetNormal();
		auto* normal_internal = static_cast<wr::d3d12::TextureResource*>(normal_handle.m_pool->GetTexture(normal_handle.m_id));

		auto roughness_handle = material_internal->GetRoughness();
		auto* roughness_internal = static_cast<wr::d3d12::TextureResource*>(roughness_handle.m_pool->GetTexture(roughness_handle.m_id));

		auto metallic_handle = material_internal->GetMetallic();
		auto* metallic_internal = static_cast<wr::d3d12::TextureResource*>(metallic_handle.m_pool->GetTexture(metallic_handle.m_id));

		d3d12::SetShaderTexture(n_cmd_list, 2, 0, albedo_internal);
		d3d12::SetShaderTexture(n_cmd_list, 2, 1, normal_internal);
		d3d12::SetShaderTexture(n_cmd_list, 2, 2, roughness_internal);
		d3d12::SetShaderTexture(n_cmd_list, 2, 3, metallic_internal);
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

	wr::Model* D3D12RenderSystem::GetSimpleShape(SimpleShapes type)
	{
		if (type == SimpleShapes::COUNT)
		{
			LOGC("Nice try boiii! That's not a shape.");
		}

		return m_simple_shapes[type];
	}

	void D3D12RenderSystem::LoadPrimitiveShapes()
	{
		// Load Cube.
		{
			wr::MeshData<wr::VertexColor> mesh;

			mesh.m_indices = {
				2, 1, 0, 3, 2, 0, 6, 5,
				4, 7, 6, 4, 10, 9, 8, 11,
				10, 8, 14, 13, 12, 15, 14, 12,
				18, 17, 16, 19, 18, 16, 22, 21,
				20, 23, 22, 20
			};

			mesh.m_vertices = {
				{ 1, 1, -1,		1, 1,		0, 0, -1,		0, 0, 0,	0, 0, 0,	0, 0, 0 },
				{ 1, -1, -1,	0, 1,		0, 0, -1,		0, 0, 0,	0, 0, 0,	0, 0, 0  },
				{ -1, -1, -1,	0, 0,		0, 0, -1,		0, 0, 0,	0, 0, 0,	0, 0, 0  },
				{ -1, 1, -1,	1, 0,		0, 0, -1,		0, 0, 0,	0, 0, 0,	0, 0, 0  },

				{ 1, 1, 1,		1, 1,		0, 0, 1,		0, 0, 0,	0, 0, 0,	0, 0, 0  },
				{ -1, 1, 1,		0, 1,		0, 0, 1,		0, 0, 0,	0, 0, 0,	0, 0, 0  },
				{ -1, -1, 1,	0, 0,		0, 0, 1,		0, 0, 0,	0, 0, 0,	0, 0, 0  },
				{ 1, -1, 1,		1, 0,		0, 0, 1,		0, 0, 0,	0, 0, 0,	0, 0, 0  },

				{ 1, 1, -1,		1, 0,		1, 0, 0,		0, 0, 0,	0, 0, 0,	0, 0, 0  },
				{ 1, 1, 1,		1, 1,		1, 0, 0,		0, 0, 0,	0, 0, 0,	0, 0, 0  },
				{ 1, -1, 1,		0, 1,		1, 0, 0,		0, 0, 0,	0, 0, 0,	0, 0, 0  },
				{ 1, -1, -1,	0, 0,		1, 0, 0,		0, 0, 0,	0, 0, 0,	0, 0, 0  },

				{ 1, -1, -1,	1, 0,		0, -1, 0,		0, 0, 0,	0, 0, 0,	0, 0, 0  },
				{ 1, -1, 1,		1, 1,		0, -1, 0,		0, 0, 0,	0, 0, 0,	0, 0, 0  },
				{ -1, -1, 1,	0, 1,		0, -1, 0,		0, 0, 0,	0, 0, 0,	0, 0, 0  },
				{ -1, -1, -1,	0, 0,		0, -1, 0,		0, 0, 0,	0, 0, 0,	0, 0, 0  },

				{ -1, -1, -1,	0, 1,		-1, 0, 0,		0, 0, 0,	0, 0, 0,	0, 0, 0  },
				{ -1, -1, 1,	0, 0,		-1, 0, 0,		0, 0, 0,	0, 0, 0,	0, 0, 0  },
				{ -1, 1, 1,		1, 0,		-1, 0, 0,		0, 0, 0,	0, 0, 0,	0, 0, 0  },
				{ -1, 1, -1,	1, 1,		-1, 0, 0,		0, 0, 0,	0, 0, 0,	0, 0, 0  },

				{ 1, 1, 1,		1, 0,		0, 1, 0,		0, 0, 0,	0, 0, 0,	0, 0, 0  },
				{ 1, 1, -1,		1, 1,		0, 1, 0,		0, 0, 0,	0, 0, 0,	0, 0, 0  },
				{ -1, 1, -1,	0, 1,		0, 1, 0,		0, 0, 0,	0, 0, 0,	0, 0, 0  },
				{ -1, 1, 1,		0, 0,		0, 1, 0,		0, 0, 0,	0, 0, 0,	0, 0, 0  },
			};

			m_simple_shapes[SimpleShapes::CUBE] = m_shapes_pool->LoadCustom<wr::VertexColor>({ mesh });
		}

		{
			wr::MeshData<wr::VertexColor> mesh;

			mesh.m_indices = {
				2, 1, 0, 3, 2, 0
			};

			mesh.m_vertices = {
				//POS				UV			NORMAL				TANGENT			BINORMAL		COLOR
				{  1,  1,  0,		1, 1,		0, 0, -1,			0, 0, 1,		0, 1, 0,		0, 0, 0 },
				{  1, -1,  0,		1, 0,		0, 0, -1,			0, 0, 1,		0, 1, 0,		0, 0, 0 },
				{ -1, -1,  0,		0, 0,		0, 0, -1,			0, 0, 1,		0, 1, 0,		0, 0, 0 },
				{ -1,  1,  0,		0, 1,		0, 0, -1,			0, 0, 1,		0, 1, 0,		0, 0, 0 },
			};

			m_simple_shapes[SimpleShapes::PLANE] = m_shapes_pool->LoadCustom<wr::VertexColor>({ mesh });
		}
	}

} /*  */

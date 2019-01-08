#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "../d3d12/d3d12_structured_buffer_pool.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../scene_graph/camera_node.hpp"
#include "../d3d12/d3d12_pipeline_registry.hpp"
#include "../engine_registry.hpp"

#include "../render_tasks/d3d12_deferred_main.hpp"

namespace wr
{
	struct DeferredCompositionTaskData
	{
		D3D12Pipeline* in_pipeline;
		d3d12::DescriptorHeap* out_srv_heap;
		d3d12::RenderTarget* out_deferred_main_rt;

		std::array<d3d12::CommandList*, d3d12::settings::num_back_buffers> out_bundle_cmd_lists;
		bool out_requires_bundle_recording;
	};

	namespace internal
	{

		inline void RecordDrawCommands(D3D12RenderSystem& render_system, d3d12::CommandList* cmd_list, d3d12::HeapResource* camera_cb, DeferredCompositionTaskData const & data, unsigned int frame_idx)
		{
			d3d12::BindComputePipeline(cmd_list, data.in_pipeline->m_native);
			

			d3d12::BindDescriptorHeaps(cmd_list, { data.out_srv_heap }, frame_idx);

			d3d12::BindComputeConstantBuffer(cmd_list, camera_cb, 0, frame_idx);

			auto gpu_handle = d3d12::GetGPUHandle(data.out_srv_heap, frame_idx);
			d3d12::BindComputeDescriptorTable(cmd_list, gpu_handle, 1);
			d3d12::Offset(gpu_handle, 4, data.out_srv_heap->m_increment_size);
			d3d12::BindComputeDescriptorTable(cmd_list, gpu_handle, 2);

			d3d12::Dispatch(cmd_list, 
				static_cast<int>(std::ceil( render_system.m_viewport.m_viewport.Width / 16.f)),
				static_cast<int>(std::ceil(render_system.m_viewport.m_viewport.Height / 16.f)),
				1);
		}

		inline void SetupDeferredCompositionTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<DeferredCompositionTaskData>(handle);

			auto& ps_registry = PipelineRegistry::Get();
			data.in_pipeline = (D3D12Pipeline*)ps_registry.Find(pipelines::deferred_composition);

			d3d12::desc::DescriptorHeapDesc heap_desc;
			heap_desc.m_shader_visible = true;
			heap_desc.m_num_descriptors = 5;
			heap_desc.m_type = DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV;
			heap_desc.m_versions = d3d12::settings::num_back_buffers;
			data.out_srv_heap = d3d12::CreateDescriptorHeap(n_render_system.m_device, heap_desc);
			SetName(data.out_srv_heap, L"Deferred Render Task SRV");

			for (uint32_t i = 0; i < d3d12::settings::num_back_buffers; ++i)
			{
				auto cpu_handle = d3d12::GetCPUHandle(data.out_srv_heap, i);

				auto deferred_main_rt = data.out_deferred_main_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<DeferredMainTaskData>());
				d3d12::CreateSRVFromRTV(deferred_main_rt, cpu_handle, 2, deferred_main_rt->m_create_info.m_rtv_formats.data());
				d3d12::CreateSRVFromDSV(deferred_main_rt, cpu_handle);

			}
		}

		inline void ExecuteDeferredCompositionTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& scene_graph, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<DeferredCompositionTaskData>(handle);
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
			auto render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);

			if (n_render_system.m_render_window.has_value())
			{
				const auto viewport = n_render_system.m_viewport;
				const auto camera_cb = static_cast<D3D12ConstantBufferHandle*>(scene_graph.GetActiveCamera()->m_camera_cb);
				const auto frame_idx = n_render_system.GetFrameIdx();

				if (static_cast<D3D12StructuredBufferHandle*>(scene_graph.GetLightBuffer())->m_native->m_states[frame_idx] != ResourceState::NON_PIXEL_SHADER_RESOURCE) 
				{
					static_cast<D3D12StructuredBufferPool*>(scene_graph.GetLightBuffer()->m_pool)->SetBufferState(scene_graph.GetLightBuffer(), ResourceState::NON_PIXEL_SHADER_RESOURCE);
					return;
				}

				//Get light buffer
				auto cpu_handle = d3d12::GetCPUHandle(data.out_srv_heap, frame_idx, 3);
				d3d12::CreateSRVFromStructuredBuffer(static_cast<D3D12StructuredBufferHandle*>(scene_graph.GetLightBuffer())->m_native, cpu_handle, frame_idx);
				std::vector<Format> formats = { Format::R8G8B8A8_UNORM };
				d3d12::CreateUAVFromRTV(render_target, cpu_handle, 1, formats.data());

				if constexpr (d3d12::settings::use_bundles)
				{
					// Record all bundles again if required.
					if (data.out_requires_bundle_recording)
					{
						for (auto i = 0; i < data.out_bundle_cmd_lists.size(); i++)
						{
							d3d12::Begin(data.out_bundle_cmd_lists[i], 0);
							RecordDrawCommands(n_render_system, data.out_bundle_cmd_lists[i], static_cast<D3D12ConstantBufferHandle*>(camera_cb)->m_native, data, i);
							d3d12::End(data.out_bundle_cmd_lists[i]);
						}
						data.out_requires_bundle_recording = false;
					}
				}
				
				//Render deferred

				d3d12::TransitionDepth(cmd_list, data.out_deferred_main_rt, ResourceState::DEPTH_WRITE, ResourceState::NON_PIXEL_SHADER_RESOURCE);

				d3d12::BindViewport(cmd_list, viewport);

				d3d12::Transition(cmd_list,
					render_target,
					wr::ResourceState::COPY_SOURCE,
					wr::ResourceState::UNORDERED_ACCESS);

				if constexpr (d3d12::settings::use_bundles)
				{
					d3d12::BindDescriptorHeaps(cmd_list, { data.out_srv_heap }, frame_idx);
					d3d12::ExecuteBundle(cmd_list, data.out_bundle_cmd_lists[frame_idx]);
				}
				else
				{						
					RecordDrawCommands(n_render_system, cmd_list, static_cast<D3D12ConstantBufferHandle*>(camera_cb)->m_native, data, frame_idx);	
				}

				d3d12::Transition(cmd_list,
					render_target,
					wr::ResourceState::UNORDERED_ACCESS,
					wr::ResourceState::COPY_SOURCE);

				d3d12::TransitionDepth(cmd_list, data.out_deferred_main_rt, ResourceState::NON_PIXEL_SHADER_RESOURCE, ResourceState::DEPTH_WRITE);
			}
		}

		inline void DestroyDeferredCompositionTask(FrameGraph& fg, RenderTaskHandle handle)
		{
			auto& data = fg.GetData<DeferredCompositionTaskData>(handle);

			d3d12::Destroy(data.out_srv_heap);
		}

	} /* internal */

	inline void AddDeferredCompositionTask(FrameGraph& fg, std::optional<unsigned int> target_width, std::optional<unsigned int> target_height)
	{
		RenderTargetProperties rt_properties
		{
			false,
			target_width,
			target_height,
			ResourceState::UNORDERED_ACCESS,
			ResourceState::COPY_SOURCE,
			false,
			Format::UNKNOWN,
			{ Format::R8G8B8A8_UNORM },
			1,
			true,
			true
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool) {
			internal::SetupDeferredCompositionTask(rs, fg, handle);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle) {
			internal::ExecuteDeferredCompositionTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph& fg, RenderTaskHandle handle, bool) {
			internal::DestroyDeferredCompositionTask(fg, handle);
		};
		desc.m_name = "Deferred Composition";
		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		fg.AddTask<DeferredCompositionTaskData>(desc);
	}

} /* wr */

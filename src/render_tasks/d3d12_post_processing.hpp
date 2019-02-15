#pragma once

#include "../frame_graph/frame_graph.hpp"
#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "d3d12_raytracing_task.hpp"

namespace wr
{
	struct PostProcessingData
	{
		d3d12::RenderTarget* out_source_rt;
		d3d12::PipelineState* out_pipeline;
		ID3D12Resource* out_previous;
		DescriptorAllocator* out_allocator;
		DescriptorAllocation out_allocation;
	};

	int versions = 1;

	namespace internal
	{

		template<typename T>
		inline void SetupPostProcessingTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<PostProcessingData>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);

			data.out_allocator = new DescriptorAllocator(n_render_system, wr::DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV);
			data.out_allocation = data.out_allocator->Allocate(2);

			auto& ps_registry = PipelineRegistry::Get();
			data.out_pipeline = ((D3D12Pipeline*)ps_registry.Find(pipelines::post_processing))->m_native;

			auto source_rt = data.out_source_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<T>());

			for (auto frame_idx = 0; frame_idx < versions; frame_idx++)
			{
				// Destination
				{
					auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::post_processing, params::PostProcessingE::DEST)));
					d3d12::CreateUAVFromSpecificRTV(n_render_target, cpu_handle, frame_idx, n_render_target->m_create_info.m_rtv_formats[frame_idx]);
				}
				// Source
				{
					auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::post_processing, params::PostProcessingE::SOURCE)));
					d3d12::CreateSRVFromSpecificRTV(source_rt, cpu_handle, frame_idx, source_rt->m_create_info.m_rtv_formats[frame_idx]);
				}
			}
		}

		inline void ExecutePostProcessingTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& device = n_render_system.m_device;
			auto& data = fg.GetData<PostProcessingData>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);
			auto frame_idx = n_render_system.GetFrameIdx();
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
			const auto viewport = n_render_system.m_viewport;

			d3d12::BindComputePipeline(cmd_list, data.out_pipeline);

			cmd_list->m_dynamic_descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(0, 0, 2, data.out_allocation.GetDescriptorHandle());

			cmd_list->m_native->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(data.out_source_rt->m_render_targets[frame_idx % versions]));

			float hdr_type = static_cast<float>(d3d12::settings::output_hdr);
			d3d12::BindCompute32BitConstants(cmd_list, &hdr_type, 1, 0, 1);

			d3d12::Dispatch(cmd_list,
				static_cast<int>(std::ceil(n_render_system.m_viewport.m_viewport.Width / 16.f)),
				static_cast<int>(std::ceil(n_render_system.m_viewport.m_viewport.Height / 16.f)),
				1);
		}

		inline void DestroyPostProcessing(FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			auto& data = fg.GetData<PostProcessingData>(handle);

			// d3d12::Destroy(data.out_srv_heap);
		}

	} /* internal */

	template<typename T>
	inline void AddPostProcessingTask(FrameGraph& frame_graph)
	{
		RenderTargetProperties rt_properties
		{
			false,
			std::nullopt,
			std::nullopt,
			ResourceState::UNORDERED_ACCESS,
			ResourceState::COPY_SOURCE,
			false,
			Format::UNKNOWN,
			{ d3d12::settings::back_buffer_format }, // Output and Previous
			1,
			false,
			false
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::SetupPostProcessingTask<T>(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle) {
			internal::ExecutePostProcessingTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::DestroyPostProcessing(fg, handle, resize);
		};
		desc.m_name = "Post Processing";
		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		frame_graph.AddTask<PostProcessingData>(desc);
	}

} /* wr */

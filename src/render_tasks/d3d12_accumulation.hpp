#pragma once

#include "../frame_graph/frame_graph.hpp"
#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "d3d12_raytracing_task.hpp"

namespace wr
{
	struct AccumulationData
	{
		d3d12::RenderTarget* out_source_rt;
		d3d12::PipelineState* out_pipeline;
		d3d12::HeapResource* out_cb;
		ID3D12Resource* out_previous;
		DescriptorAllocator* out_allocator;
		DescriptorAllocation out_allocation;
	};

	int versions = 1;

	namespace internal
	{

		inline void SetupAccumulationTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<AccumulationData>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);
			d3d12::SetName(n_render_target, L"Accumulation RT");

			data.out_allocator = new DescriptorAllocator(n_render_system, wr::DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV);
			data.out_allocation = data.out_allocator->Allocate(1);

			auto& ps_registry = PipelineRegistry::Get();
			data.out_pipeline = ((D3D12Pipeline*)ps_registry.Find(pipelines::accumulation))->m_native;

			auto source_rt = data.out_source_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<RaytracingData>());

			auto& source_data = fg.GetPredecessorData<RaytracingData>();
			data.out_cb = source_data.out_cb_camera_handle->m_native;

			for (auto frame_idx = 0; frame_idx < versions; frame_idx++)
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle();

				d3d12::CreateSRVFromSpecificRTV(source_rt, cpu_handle, frame_idx, source_rt->m_create_info.m_rtv_formats[frame_idx]);
			}
		}

		inline void ExecuteAccumulationTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& device = n_render_system.m_device;
			auto& data = fg.GetData<AccumulationData>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);
			auto frame_idx = n_render_system.GetFrameIdx();
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
			const auto viewport = n_render_system.m_viewport;

			d3d12::BindPipeline(cmd_list, data.out_pipeline);

			d3d12::BindViewport(cmd_list, viewport);

			cmd_list->m_dynamic_descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(0, 0, 1, data.out_allocation.GetDescriptorHandle());

			d3d12::BindConstantBuffer(cmd_list, data.out_cb, 1, frame_idx);

			cmd_list->m_native->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(data.out_source_rt->m_render_targets[frame_idx % versions]));

			d3d12::SetPrimitiveTopology(cmd_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
			d3d12::BindVertexBuffer(cmd_list, n_render_system.m_fullscreen_quad_vb, 0, n_render_system.m_fullscreen_quad_vb->m_size, sizeof(Vertex2D));

			d3d12::Draw(cmd_list, 4, 1, 0);
		}

		inline void DestroyAccumulationTask(FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			auto& data = fg.GetData<AccumulationData>(handle);

			// d3d12::Destroy(data.out_srv_heap);
		}

	} /* internal */

	inline void AddAccumulationTask(FrameGraph& frame_graph)
	{
		RenderTargetProperties rt_properties
		{
			false,
			std::nullopt,
			std::nullopt,
			ResourceState::RENDER_TARGET,
			ResourceState::COPY_SOURCE,
			false,
			Format::UNKNOWN,
			{ Format::R8G8B8A8_UNORM }, // Output and Previous
			1,
			false,
			false
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::SetupAccumulationTask(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle) {
			internal::ExecuteAccumulationTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::DestroyAccumulationTask(fg, handle, resize);
		};
		desc.m_name = "Accumulation";
		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::DIRECT;
		desc.m_allow_multithreading = true;

		frame_graph.AddTask<AccumulationData>(desc);
	}

} /* wr */

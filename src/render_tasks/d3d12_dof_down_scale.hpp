#pragma once

#include "../frame_graph/frame_graph.hpp"
#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "../render_tasks/d3d12_deferred_main.hpp"
#include "../render_tasks/d3d12_post_processing.hpp"

#include "d3d12_raytracing_task.hpp"

namespace wr
{
	struct DoFDownScaleData
	{
		d3d12::RenderTarget* out_source_rt;
		d3d12::RenderTarget* out_source_coc;
		d3d12::PipelineState* out_pipeline;
		ID3D12Resource* out_previous;
		DescriptorAllocator* out_allocator;
		DescriptorAllocation out_allocation;
	};

	namespace internal
	{
	
		template<typename T, typename T1>
		inline void SetupDoFDownScaleTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<DoFDownScaleData>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);

			data.out_allocator = new DescriptorAllocator(n_render_system, wr::DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV);
			data.out_allocation = data.out_allocator->Allocate(4);

			auto& ps_registry = PipelineRegistry::Get();
			data.out_pipeline = ((D3D12Pipeline*)ps_registry.Find(pipelines::dof_down_scale))->m_native;

			auto source_rt = data.out_source_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<T>());
			auto source_coc = data.out_source_coc = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<T1>());

			for (auto frame_idx = 0; frame_idx < versions; frame_idx++)
			{
				// Destination near
				{
					auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::dof_down_scale, params::DoFDownScaleE::OUTPUT_NEAR)));
					d3d12::CreateUAVFromSpecificRTV(n_render_target, cpu_handle, 0, n_render_target->m_create_info.m_rtv_formats[0]);
				}
				// Destination far
				{
					auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::dof_down_scale, params::DoFDownScaleE::OUTPUT_FAR)));
					d3d12::CreateUAVFromSpecificRTV(n_render_target, cpu_handle, 1, n_render_target->m_create_info.m_rtv_formats[1]);
				}
				// Source
				{
					auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::dof_down_scale, params::DoFDownScaleE::SOURCE)));
					d3d12::CreateSRVFromSpecificRTV(source_rt, cpu_handle, frame_idx, source_rt->m_create_info.m_rtv_formats[frame_idx]);
				}
				// Cone of confusion
				{
					auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::dof_down_scale, params::DoFDownScaleE::COC)));
					d3d12::CreateSRVFromSpecificRTV(source_coc, cpu_handle, frame_idx, source_coc->m_create_info.m_rtv_formats[frame_idx]);
				}
			}
		}

		inline void ExecuteDoFDownScaleTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& device = n_render_system.m_device;
			auto& data = fg.GetData<DoFDownScaleData>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);
			auto frame_idx = n_render_system.GetFrameIdx();
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
			const auto viewport = n_render_system.m_viewport;

			d3d12::BindComputePipeline(cmd_list, data.out_pipeline);

			cmd_list->m_dynamic_descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(0, 0, 4, data.out_allocation.GetDescriptorHandle());

			cmd_list->m_native->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(data.out_source_rt->m_render_targets[frame_idx % versions]));

			d3d12::Dispatch(cmd_list,
				static_cast<int>(std::ceil(n_render_system.m_viewport.m_viewport.Width / 16.f)),
				static_cast<int>(std::ceil(n_render_system.m_viewport.m_viewport.Height / 16.f)),
				1);
		}

	} /* internal */

	template<typename T, typename T1>
	inline void AddDoFDownScaleTask(FrameGraph& frame_graph, int32_t width, int32_t height)
	{
		const std::uint32_t m_half_width = (uint32_t)width / 2;
		const std::uint32_t m_half_height = (uint32_t)height / 2;

		std::wstring name(L"DoF down scale");

		RenderTargetProperties rt_properties
		{
			RenderTargetProperties::IsRenderWindow(false),
			RenderTargetProperties::Width(m_half_width),
			RenderTargetProperties::Height(m_half_height),
			RenderTargetProperties::ExecuteResourceState(ResourceState::UNORDERED_ACCESS),
			RenderTargetProperties::FinishedResourceState(ResourceState::COPY_SOURCE),
			RenderTargetProperties::CreateDSVBuffer(false),
			RenderTargetProperties::DSVFormat(Format::UNKNOWN),
			RenderTargetProperties::RTVFormats({ wr::Format::R16G16B16A16_FLOAT, wr::Format::R16G16B16A16_FLOAT}),
			RenderTargetProperties::NumRTVFormats(2),
			RenderTargetProperties::Clear(false),
			RenderTargetProperties::ClearDepth(false),
			RenderTargetProperties::ResourceName(name),
			RenderTargetProperties::ResolutionScalar(0.5f)
		};

		RenderTaskDesc desc; 
		desc.m_setup_func = [](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::SetupDoFDownScaleTask<T, T1>(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle) {
			internal::ExecuteDoFDownScaleTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph& fg, RenderTaskHandle handle, bool resize) {
		};
		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		frame_graph.AddTask<DoFDownScaleData>(desc);
	}

} /* wr */

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
	struct DoFCoCData
	{
		d3d12::RenderTarget* out_source_dsv;
		d3d12::PipelineState* out_pipeline;
		ID3D12Resource* out_previous;

		std::shared_ptr<ConstantBufferPool> camera_cb_pool;
		D3D12ConstantBufferHandle* cb_handle;

		DescriptorAllocator* out_allocator;
		DescriptorAllocation out_allocation;
	};

	namespace internal
	{
		struct DoFProperties_CB
		{
			DirectX::XMMATRIX m_projection;
			float m_focal_length;
			float m_f_number;
			float m_film_size;
			float m_focus_dist;
			int m_enable_dof;
			//uint32_t m_blades;
		};

		template<typename T>
		inline void SetupDoFCoCTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<DoFCoCData>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);

			data.out_allocator = new DescriptorAllocator(n_render_system, wr::DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV);
			data.out_allocation = data.out_allocator->Allocate(2);

			data.camera_cb_pool = rs.CreateConstantBufferPool(2);
			data.cb_handle = static_cast<D3D12ConstantBufferHandle*>(data.camera_cb_pool->Create(sizeof(DoFProperties_CB)));

			auto& ps_registry = PipelineRegistry::Get();
			data.out_pipeline = ((D3D12Pipeline*)ps_registry.Find(pipelines::dof_coc))->m_native;

			auto source_dsv = data.out_source_dsv = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<T>());

			for (auto frame_idx = 0; frame_idx < versions; frame_idx++)
			{
				// Destination
				{
					auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::dof_coc, params::DoFCoCE::OUTPUT)));
					d3d12::CreateUAVFromSpecificRTV(n_render_target, cpu_handle, frame_idx, n_render_target->m_create_info.m_rtv_formats[frame_idx]);
				}
				// Source
				{
					auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::dof_coc, params::DoFCoCE::GDEPTH)));
					d3d12::CreateSRVFromDSV(source_dsv, cpu_handle);
				}
			}
		}

		inline void ExecuteDoFCoCTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& device = n_render_system.m_device;
			auto& data = fg.GetData<DoFCoCData>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);
			auto frame_idx = n_render_system.GetFrameIdx();
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
			const auto viewport = n_render_system.m_viewport;

			const auto camera_cb = static_cast<D3D12ConstantBufferHandle*>(sg.GetActiveCamera()->m_camera_cb);

			DoFProperties_CB cb_data;
			cb_data.m_projection = sg.GetActiveCamera()->m_projection;
			cb_data.m_film_size = sg.GetActiveCamera()->m_film_size;
			cb_data.m_focal_length = sg.GetActiveCamera()->m_focal_length;
			cb_data.m_f_number = sg.GetActiveCamera()->m_f_number;
			cb_data.m_focus_dist = sg.GetActiveCamera()->m_focus_dist;
			cb_data.m_enable_dof = sg.GetActiveCamera()->m_enable_dof;

			data.cb_handle->m_pool->Update(data.cb_handle, sizeof(DoFProperties_CB), 0, frame_idx, (uint8_t*)&cb_data);

			//d3d12::BindConstantBuffer(cmd_list, data.cb_handle->m_native, 2, frame_idx);
			

			d3d12::BindComputePipeline(cmd_list, data.out_pipeline);

			d3d12::BindComputeConstantBuffer(cmd_list, data.cb_handle->m_native, 1, frame_idx);

			cmd_list->m_dynamic_descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(0, 0, 2, data.out_allocation.GetDescriptorHandle());

			cmd_list->m_native->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(data.out_source_dsv->m_render_targets[frame_idx % versions]));


			//TODO: numthreads is currently hardcoded to half resolution, change when dimensions are done properly
			d3d12::Dispatch(cmd_list,
				static_cast<int>(std::ceil(n_render_system.m_viewport.m_viewport.Width / 16.f)),
				static_cast<int>(std::ceil(n_render_system.m_viewport.m_viewport.Height / 16.f)),
				1);
		}

	} /* internal */

	template<typename T>
	inline void AddDoFCoCTask(FrameGraph& frame_graph)
	{
		RenderTargetProperties rt_properties
		{
			RenderTargetProperties::IsRenderWindow(false),
			RenderTargetProperties::Width(std::nullopt),
			RenderTargetProperties::Height(std::nullopt),
			RenderTargetProperties::ExecuteResourceState(ResourceState::UNORDERED_ACCESS),
			RenderTargetProperties::FinishedResourceState(ResourceState::COPY_SOURCE),
			RenderTargetProperties::CreateDSVBuffer(false),
			RenderTargetProperties::DSVFormat(Format::UNKNOWN),
			RenderTargetProperties::RTVFormats({ wr::Format::R32_FLOAT }),
			RenderTargetProperties::NumRTVFormats(1),
			RenderTargetProperties::Clear(false),
			RenderTargetProperties::ClearDepth(false)
		};

		RenderTaskDesc desc; 
		desc.m_setup_func = [](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::SetupDoFCoCTask<T>(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle) {
			internal::ExecuteDoFCoCTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph& fg, RenderTaskHandle handle, bool resize) {
		};
		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		frame_graph.AddTask<DoFCoCData>(desc);
	}

} /* wr */

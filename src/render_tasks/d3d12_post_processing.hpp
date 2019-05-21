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
		d3d12::RenderTarget* out_source_rt = nullptr;
		d3d12::PipelineState* out_pipeline = nullptr;

		DescriptorAllocator* out_allocator = nullptr;
		DescriptorAllocation out_allocation;
	};

	int versions = 1;

	namespace internal
	{

		template<typename T>
		inline void SetupPostProcessingTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<PostProcessingData>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);

			//Retrieve the texture pool from the render system. It will be used to allocate temporary cpu visible descriptors
			std::shared_ptr<D3D12TexturePool> texture_pool = std::static_pointer_cast<D3D12TexturePool>(n_render_system.m_texture_pools[0]);
			if (!texture_pool)
			{
				LOGC("Texture pool is nullptr. This shouldn't happen as the render system should always create the first texture pool");
			}


			data.out_allocator = texture_pool->GetAllocator(DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV);
			data.out_allocation = data.out_allocator->Allocate(2);

			auto& ps_registry = PipelineRegistry::Get();
			data.out_pipeline = ((D3D12Pipeline*)ps_registry.Find(pipelines::post_processing))->m_native;

			auto source_rt = data.out_source_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<T>());

			// Destination
			{
				constexpr unsigned int dest_idx = rs_layout::GetHeapLoc(params::post_processing, params::PostProcessingE::DEST);
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(dest_idx);
				d3d12::CreateUAVFromSpecificRTV(n_render_target, cpu_handle, 0, n_render_target->m_create_info.m_rtv_formats[0]);
			}
			// Source
			{
				constexpr unsigned int source_idx = rs_layout::GetHeapLoc(params::post_processing, params::PostProcessingE::SOURCE);
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(source_idx);
				d3d12::CreateSRVFromSpecificRTV(source_rt, cpu_handle, 0, source_rt->m_create_info.m_rtv_formats[0]);
			}

		}

		template<typename T>
		inline void ExecutePostProcessingTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& device = n_render_system.m_device;
			auto& data = fg.GetData<PostProcessingData>(handle);
			auto frame_idx = n_render_system.GetFrameIdx();
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);

			d3d12::BindComputePipeline(cmd_list, data.out_pipeline);

			auto source_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<T>());

			// Destination
			{
				constexpr unsigned int dest_idx = rs_layout::GetHeapLoc(params::post_processing, params::PostProcessingE::DEST);
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(dest_idx);
				d3d12::CreateUAVFromSpecificRTV(n_render_target, cpu_handle, 0, n_render_target->m_create_info.m_rtv_formats[0]);
			}
			// Source
			{
				constexpr unsigned int source_idx = rs_layout::GetHeapLoc(params::post_processing, params::PostProcessingE::SOURCE);
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(source_idx);
				d3d12::CreateSRVFromSpecificRTV(source_rt, cpu_handle, 0, source_rt->m_create_info.m_rtv_formats[0]);
			}
			
			auto hdr_type = static_cast<float>(d3d12::settings::output_hdr);
			d3d12::BindCompute32BitConstants(cmd_list, &hdr_type, 1, 0, 1);

			constexpr unsigned int dest_idx = rs_layout::GetHeapLoc(params::post_processing, params::PostProcessingE::DEST);
			auto handle_uav = data.out_allocation.GetDescriptorHandle(dest_idx);
			d3d12::SetShaderUAV(cmd_list, 0, dest_idx, handle_uav);

			constexpr unsigned int source_idx = rs_layout::GetHeapLoc(params::post_processing, params::PostProcessingE::SOURCE);
			auto handle_srv = data.out_allocation.GetDescriptorHandle(source_idx);
			d3d12::SetShaderSRV(cmd_list, 0, source_idx, handle_srv);

			cmd_list->m_native->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(data.out_source_rt->m_render_targets[frame_idx % versions]));

			d3d12::Dispatch(cmd_list,
				static_cast<int>(std::ceil(n_render_system.m_viewport.m_viewport.Width / 16.f)),
				static_cast<int>(std::ceil(n_render_system.m_viewport.m_viewport.Height / 16.f)),
				1);
		}

		inline void DestroyPostProcessing(FrameGraph& fg, RenderTaskHandle handle, bool)
		{
			auto& data = fg.GetData<PostProcessingData>(handle);

			// Small hack to force the allocations to go out of scope, which will tell the texture pool to free them
			DescriptorAllocation temp = std::move(data.out_allocation);
		}

	} /* internal */

	template<typename T>
	inline void AddPostProcessingTask(FrameGraph& frame_graph)
	{
		std::wstring name(L"Post Processing");

		RenderTargetProperties rt_properties
		{
			RenderTargetProperties::IsRenderWindow(false),
			RenderTargetProperties::Width(std::nullopt),
			RenderTargetProperties::Height(std::nullopt),
			RenderTargetProperties::ExecuteResourceState(ResourceState::UNORDERED_ACCESS),
			RenderTargetProperties::FinishedResourceState(ResourceState::COPY_SOURCE),
			RenderTargetProperties::CreateDSVBuffer(false),
			RenderTargetProperties::DSVFormat(Format::UNKNOWN),
			RenderTargetProperties::RTVFormats({d3d12::settings::back_buffer_format}),
			RenderTargetProperties::NumRTVFormats(1),
			RenderTargetProperties::Clear(false),
			RenderTargetProperties::ClearDepth(false),
			RenderTargetProperties::ResourceName(name)
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::SetupPostProcessingTask<T>(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle) {
			internal::ExecutePostProcessingTask<T>(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::DestroyPostProcessing(fg, handle, resize);
		};

		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::DIRECT;
		desc.m_allow_multithreading = true;

		frame_graph.AddTask<PostProcessingData>(desc);
	}

} /* wr */

#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_defines.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_pipeline_registry.hpp"
#include "../engine_registry.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../d3d12/d3d12_descriptors_allocations.hpp"
#include "../d3d12/d3d12_resource_pool_texture.hpp"

namespace wr
{
	struct BrdfLutTaskData
	{
		D3D12Pipeline* in_pipeline;

		DescriptorAllocator* out_allocator;
		DescriptorAllocation out_rtv_uav_allocation;

		bool should_run;
	};

	namespace internal
	{
		inline void SetupBrdfLutPrecalculationTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			if (!resize)
			{
				auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
				auto& data = fg.GetData<BrdfLutTaskData>(handle);
				auto render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);

				auto& ps_registry = PipelineRegistry::Get();
				data.in_pipeline = (D3D12Pipeline*)ps_registry.Find(pipelines::brdf_lut_precalculation);

				//Retrieve the texture pool from the render system. It will be used to allocate temporary cpu visible descriptors
				std::shared_ptr<D3D12TexturePool> texture_pool = std::static_pointer_cast<D3D12TexturePool>(n_render_system.m_texture_pools[0]);
				if (!texture_pool)
				{
					LOGC("Texture pool is nullptr. This shouldn't happen as the render system should always create the first texture pool");
				}

				data.out_allocator = texture_pool->GetAllocator(DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV);
				data.out_rtv_uav_allocation = std::move(data.out_allocator->Allocate(1));

				auto rtv_uav_handle = data.out_rtv_uav_allocation.GetDescriptorHandle();
				d3d12::CreateUAVFromSpecificRTV(render_target, rtv_uav_handle, 0, wr::Format::R16G16_FLOAT);

				data.should_run = true;
			}
		}

		inline void ExecuteBrdfLutPrecalculationTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& scene_graph, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<BrdfLutTaskData>(handle);

			if (data.should_run)
			{
				auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
				auto render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);

				d3d12::BindComputePipeline(cmd_list, data.in_pipeline->m_native);

				//d3d12::Transition(cmd_list, render_target, ResourceState::RENDER_TARGET, ResourceState::UNORDERED_ACCESS);

				auto rtv_uav_handle = data.out_rtv_uav_allocation.GetDescriptorHandle();
				d3d12::SetShaderUAV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::brdf_lut, params::BRDF_LutE::OUTPUT)), rtv_uav_handle);

				d3d12::Dispatch(cmd_list, static_cast<int>(512 / 16), static_cast<int>(512 / 16.f), 1);

				//d3d12::Transition(cmd_list, render_target, ResourceState::UNORDERED_ACCESS, ResourceState::PIXEL_SHADER_RESOURCE);

				data.should_run = false;
			}
		}

		inline void DestroyBrdfLutPrecalculationTask(FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			if (!resize)
			{
				auto& data = fg.GetData<BrdfLutTaskData>(handle);

				DescriptorAllocation temp1 = std::move(data.out_rtv_uav_allocation);
			}
		}
	}

	inline void AddBrdfLutPrecalculationTask(FrameGraph& fg)
	{
		RenderTargetProperties rt_properties
		{
			RenderTargetProperties::IsRenderWindow(false),
			RenderTargetProperties::Width(512),
			RenderTargetProperties::Height(512),
			RenderTargetProperties::ExecuteResourceState(ResourceState::UNORDERED_ACCESS),
			RenderTargetProperties::FinishedResourceState(ResourceState::PIXEL_SHADER_RESOURCE),
			RenderTargetProperties::CreateDSVBuffer(false),
			RenderTargetProperties::DSVFormat(Format::UNKNOWN),
			RenderTargetProperties::RTVFormats({ Format::R16G16_FLOAT }),
			RenderTargetProperties::NumRTVFormats(1),
			RenderTargetProperties::Clear(true),
			RenderTargetProperties::ClearDepth(false)
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::SetupBrdfLutPrecalculationTask(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle) {
			internal::ExecuteBrdfLutPrecalculationTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::DestroyBrdfLutPrecalculationTask(fg, handle, resize);
		};
		desc.m_name = "BRDF LUT Precalculation";
		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		fg.AddTask<BrdfLutTaskData>(desc);
	}
}
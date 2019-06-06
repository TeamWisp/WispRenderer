#pragma once
#include "../render_tasks/d3d12_spatial_reconstruction.hpp"
#include "../render_tasks/d3d12_deferred_main.hpp"
#include "../render_tasks/d3d12_rt_reflection_task.hpp"

#include "../frame_graph/frame_graph.hpp"
#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"

namespace wr
{
	struct ReflectionDenoiserData
	{
		d3d12::PipelineState* m_spatial_denoiser_pipeline;

		DescriptorAllocator* m_descriptor_allocator;
		DescriptorAllocation m_input_allocation;
		DescriptorAllocation m_ray_raw_allocation;
		DescriptorAllocation m_ray_dir_allocation;
		DescriptorAllocation m_albedo_roughness_allocation;
		DescriptorAllocation m_normal_metallic_allocation;
		DescriptorAllocation m_motion_allocation;
		DescriptorAllocation m_linear_depth_allocation;
		DescriptorAllocation m_world_pos_allocation;
		DescriptorAllocation m_output_allocation;

		d3d12::RenderTarget* m_spatial_reconstruction_render_target;
		d3d12::RenderTarget* m_rt_reflection_render_target;
		d3d12::RenderTarget* m_gbuffer_render_target;
		d3d12::RenderTarget* m_output_render_target;
	};

	namespace internal
	{
		inline void SetupReflectionDenoiserTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<ReflectionDenoiserData>(handle);

			data.m_output_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);
			data.m_spatial_reconstruction_render_target = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<SpatialReconstructionData>());
			data.m_rt_reflection_render_target = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<RTReflectionData>());
			data.m_gbuffer_render_target = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<DeferredMainTaskData>());

			//Retrieve the texture pool from the render system. It will be used to allocate temporary cpu visible descriptors
			std::shared_ptr<D3D12TexturePool> texture_pool = std::static_pointer_cast<D3D12TexturePool>(n_render_system.m_texture_pools[0]);
			if (!texture_pool)
			{
				LOGC("Texture pool is nullptr. This shouldn't happen as the render system should always create the first texture pool");
			}
			if (!resize)
			{
				data.m_descriptor_allocator = texture_pool->GetAllocator(DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV);

				data.m_input_allocation = std::move(data.m_descriptor_allocator->Allocate());
				data.m_ray_raw_allocation = std::move(data.m_descriptor_allocator->Allocate());
				data.m_ray_dir_allocation = std::move(data.m_descriptor_allocator->Allocate());
				data.m_albedo_roughness_allocation = std::move(data.m_descriptor_allocator->Allocate());
				data.m_normal_metallic_allocation = std::move(data.m_descriptor_allocator->Allocate());
				data.m_motion_allocation = std::move(data.m_descriptor_allocator->Allocate());
				data.m_linear_depth_allocation = std::move(data.m_descriptor_allocator->Allocate());
				data.m_world_pos_allocation = std::move(data.m_descriptor_allocator->Allocate());
				data.m_output_allocation = std::move(data.m_descriptor_allocator->Allocate());
			}

			{
				auto cpu_handle = data.m_input_allocation.GetDescriptorHandle();
				d3d12::CreateSRVFromSpecificRTV(data.m_spatial_reconstruction_render_target, cpu_handle, 0, data.m_spatial_reconstruction_render_target->m_create_info.m_rtv_formats[0]);
			}

			{
				auto cpu_handle = data.m_ray_raw_allocation.GetDescriptorHandle();
				d3d12::CreateSRVFromSpecificRTV(data.m_rt_reflection_render_target, cpu_handle, 0, data.m_rt_reflection_render_target->m_create_info.m_rtv_formats[0]);
			}

			{
				auto cpu_handle = data.m_ray_dir_allocation.GetDescriptorHandle();
				d3d12::CreateSRVFromSpecificRTV(data.m_rt_reflection_render_target, cpu_handle, 2, data.m_rt_reflection_render_target->m_create_info.m_rtv_formats[2]);
			}

			{
				auto cpu_handle = data.m_albedo_roughness_allocation.GetDescriptorHandle();
				d3d12::CreateSRVFromSpecificRTV(data.m_gbuffer_render_target, cpu_handle, 0, data.m_gbuffer_render_target->m_create_info.m_rtv_formats[0]);
			}

			{
				auto cpu_handle = data.m_normal_metallic_allocation.GetDescriptorHandle();
				d3d12::CreateSRVFromSpecificRTV(data.m_gbuffer_render_target, cpu_handle, 1, data.m_gbuffer_render_target->m_create_info.m_rtv_formats[1]);
			}

			{
				auto cpu_handle = data.m_motion_allocation.GetDescriptorHandle();
				d3d12::CreateSRVFromSpecificRTV(data.m_gbuffer_render_target, cpu_handle, 3, data.m_gbuffer_render_target->m_create_info.m_rtv_formats[3]);
			}

			{
				auto cpu_handle = data.m_linear_depth_allocation.GetDescriptorHandle();
				d3d12::CreateSRVFromSpecificRTV(data.m_gbuffer_render_target, cpu_handle, 4, data.m_gbuffer_render_target->m_create_info.m_rtv_formats[4]);
			}

			{
				auto cpu_handle = data.m_world_pos_allocation.GetDescriptorHandle();
				d3d12::CreateSRVFromSpecificRTV(data.m_gbuffer_render_target, cpu_handle, 5, data.m_gbuffer_render_target->m_create_info.m_rtv_formats[5]);
			}

			{
				auto cpu_handle = data.m_output_allocation.GetDescriptorHandle();
				d3d12::CreateUAVFromSpecificRTV(data.m_output_render_target, cpu_handle, 0, data.m_output_render_target->m_create_info.m_rtv_formats[0]);
			}

			auto& ps_registry = PipelineRegistry::Get();
			data.m_spatial_denoiser_pipeline = (d3d12::PipelineState*)ps_registry.Find(pipelines::reflection_spatial_denoiser);
		}

		inline void ExecuteReflectionDenoiserTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& n_device = n_render_system.m_device->m_native;
			auto& data = fg.GetData<ReflectionDenoiserData>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);
			auto frame_idx = n_render_system.GetFrameIdx();
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
			const auto viewport = n_render_system.m_viewport;

			fg.WaitForPredecessorTask<SpatialReconstructionData>();

			d3d12::BindComputePipeline(cmd_list, data.m_spatial_denoiser_pipeline);

			{
				constexpr unsigned int input = rs_layout::GetHeapLoc(params::reflection_denoiser, params::ReflectionDenoiserE::INPUT);
				auto cpu_handle = data.m_input_allocation.GetDescriptorHandle();
				d3d12::SetShaderSRV(cmd_list, 0, input, cpu_handle);
			}

			{
				constexpr unsigned int ray_raw = rs_layout::GetHeapLoc(params::reflection_denoiser, params::ReflectionDenoiserE::RAY_RAW);
				auto cpu_handle = data.m_ray_raw_allocation.GetDescriptorHandle();
				d3d12::SetShaderSRV(cmd_list, 0, ray_raw, cpu_handle);
			}

			{
				constexpr unsigned int ray_dir = rs_layout::GetHeapLoc(params::reflection_denoiser, params::ReflectionDenoiserE::RAY_DIR);
				auto cpu_handle = data.m_ray_dir_allocation.GetDescriptorHandle();
				d3d12::SetShaderSRV(cmd_list, 0, ray_dir, cpu_handle);
			}

			{
				constexpr unsigned int albedo_roughness = rs_layout::GetHeapLoc(params::reflection_denoiser, params::ReflectionDenoiserE::ALBEDO_ROUGHNESS);
				auto cpu_handle = data.m_albedo_roughness_allocation.GetDescriptorHandle();
				d3d12::SetShaderSRV(cmd_list, 0, albedo_roughness, cpu_handle);
			}

			{
				constexpr unsigned int normal_metallic = rs_layout::GetHeapLoc(params::reflection_denoiser, params::ReflectionDenoiserE::NORMAL_METALLIC);
				auto cpu_handle = data.m_normal_metallic_allocation.GetDescriptorHandle();
				d3d12::SetShaderSRV(cmd_list, 0, normal_metallic, cpu_handle);
			}

			{
				constexpr unsigned int motion = rs_layout::GetHeapLoc(params::reflection_denoiser, params::ReflectionDenoiserE::MOTION);
				auto cpu_handle = data.m_motion_allocation.GetDescriptorHandle();
				d3d12::SetShaderSRV(cmd_list, 0, motion, cpu_handle);
			}

			{
				constexpr unsigned int linear_depth = rs_layout::GetHeapLoc(params::reflection_denoiser, params::ReflectionDenoiserE::LINEAR_DEPTH);
				auto cpu_handle = data.m_linear_depth_allocation.GetDescriptorHandle();
				d3d12::SetShaderSRV(cmd_list, 0, linear_depth, cpu_handle);
			}

			{
				constexpr unsigned int world_pos = rs_layout::GetHeapLoc(params::reflection_denoiser, params::ReflectionDenoiserE::WORLD_POS);
				auto cpu_handle = data.m_world_pos_allocation.GetDescriptorHandle();
				d3d12::SetShaderSRV(cmd_list, 0, world_pos, cpu_handle);
			}

			{
				constexpr unsigned int output = rs_layout::GetHeapLoc(params::reflection_denoiser, params::ReflectionDenoiserE::OUTPUT);
				auto cpu_handle = data.m_output_allocation.GetDescriptorHandle();
				d3d12::SetShaderUAV(cmd_list, 0, output, cpu_handle);
			}

			d3d12::HeapResource* camera_buffer = static_cast<D3D12ConstantBufferHandle*>(sg.GetActiveCamera()->m_camera_cb)->m_native;

			d3d12::BindComputeConstantBuffer(cmd_list, camera_buffer, 1, frame_idx);

			d3d12::Transition(cmd_list, data.m_output_render_target, ResourceState::COPY_SOURCE, ResourceState::UNORDERED_ACCESS);

			d3d12::Dispatch(cmd_list,
				uint32_t(std::ceil(viewport.m_viewport.Width / 16.f)),
				uint32_t(std::ceil(viewport.m_viewport.Height / 16.f)),
				1);

			d3d12::Transition(cmd_list, data.m_output_render_target, ResourceState::UNORDERED_ACCESS, ResourceState::COPY_SOURCE);
		}

		inline void DestroyReflectionDenoiserTask(FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			auto& data = fg.GetData<ReflectionDenoiserData>(handle);
			if (!resize)
			{
				std::move(data.m_input_allocation);
				std::move(data.m_ray_dir_allocation);
				std::move(data.m_albedo_roughness_allocation);
				std::move(data.m_normal_metallic_allocation);
				std::move(data.m_motion_allocation);
				std::move(data.m_linear_depth_allocation);
				std::move(data.m_world_pos_allocation);
				std::move(data.m_output_allocation);
			}
		}
	} /* internal */

	inline void AddReflectionDenoiserTask(FrameGraph& frame_graph)
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
		  RenderTargetProperties::RTVFormats({ wr::Format::R16G16B16A16_FLOAT }),
		  RenderTargetProperties::NumRTVFormats(1),
		  RenderTargetProperties::Clear(false),
		  RenderTargetProperties::ClearDepth(false),
		  RenderTargetProperties::ResolutionScalar(1.f)
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::SetupReflectionDenoiserTask(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle) {
			internal::ExecuteReflectionDenoiserTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::DestroyReflectionDenoiserTask(fg, handle, resize);
		};
		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		frame_graph.AddTask<ReflectionDenoiserData>(desc, L"Reflection denoiser", FG_DEPS<SpatialReconstructionData>());
	}

} /* wr */

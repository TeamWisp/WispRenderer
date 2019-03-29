#pragma once
#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "../d3d12/d3d12_structured_buffer_pool.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../scene_graph/scene_graph.hpp"
#include "../scene_graph/camera_node.hpp"
#include "../d3d12/d3d12_rt_pipeline_registry.hpp"
#include "../d3d12/d3d12_root_signature_registry.hpp"
#include "../engine_registry.hpp"
#include "../util/math.hpp"

#include "../render_tasks/d3d12_deferred_main.hpp"

#include <optional>

namespace wr
{
	struct ShadowDenoiserData
	{
		bool m_has_depth_buffer;
		d3d12::RenderTarget* m_depth_buffer;
		d3d12::RenderTarget* m_noisy_buffer;
		d3d12::RenderTarget* m_output_buffer;

		wr::D3D12Pipeline* m_pipeline;

		DescriptorAllocator* out_allocator;
		DescriptorAllocation out_allocation;

		std::shared_ptr<ConstantBufferPool> m_constant_buffer_pool;

		temp::ShadowDenoiserSettings_CBData m_denoiser_settings;
		ConstantBufferHandle* m_denoiser_settings_buffer_horizontal;
		ConstantBufferHandle* m_denoiser_settings_buffer_vertical;

		TextureHandle m_kernel;
	};

	namespace internal
	{
		inline float CalculateGaussian(float height, float center, float width, float offset)
		{
			float exponent = -(pow(offset - center, 2.f) / (2.f * pow(width, 2.f)));
			return height * exp(exponent);

		}

		inline void SetupShadowDenoiserTask(RenderSystem& rs, FrameGraph&  fg, RenderTaskHandle handle, bool resize, std::optional<std::string_view> kernel_texture_location)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<ShadowDenoiserData>(handle);

			auto& ps_registry = PipelineRegistry::Get();
			data.m_pipeline = (D3D12Pipeline*)ps_registry.Find(pipelines::shadow_denoiser);

			//Retrieve the texture pool from the render system. It will be used to allocate temporary cpu visible descriptors
			std::shared_ptr<D3D12TexturePool> texture_pool = std::static_pointer_cast<D3D12TexturePool>(n_render_system.m_texture_pools[0]);
			if (!texture_pool)
			{
				LOGC("Texture pool is nullptr. This shouldn't happen as the render system should always create the first texture pool");
			}

			data.out_allocator = texture_pool->GetAllocator(DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV);
			data.out_allocation = std::move(data.out_allocator->Allocate(4));

			data.m_noisy_buffer = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<wr::RTShadowData>());
			data.m_output_buffer = fg.GetRenderTarget<d3d12::RenderTarget>(handle);

			if (fg.HasTask<wr::DeferredMainTaskData>())
			{
				data.m_has_depth_buffer = true;
				data.m_depth_buffer = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<wr::DeferredMainTaskData>());
			}

			// Dest
			{
				constexpr unsigned int dest_idx = rs_layout::GetHeapLoc(params::shadow_denoiser, params::ShadowDenoiserE::DEST);
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(dest_idx);
				d3d12::CreateUAVFromRTV(data.m_output_buffer, cpu_handle, 1, data.m_output_buffer->m_create_info.m_rtv_formats.data());

			}
			// Source
			{
				constexpr unsigned int source_idx = rs_layout::GetHeapLoc(params::shadow_denoiser, params::ShadowDenoiserE::SOURCE);
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(source_idx);
				d3d12::CreateSRVFromRTV(data.m_noisy_buffer, cpu_handle, 1, data.m_noisy_buffer->m_create_info.m_rtv_formats.data());
			}

			// Depth
			if (data.m_has_depth_buffer)
			{
				constexpr unsigned int depth_idx = rs_layout::GetHeapLoc(params::shadow_denoiser, params::ShadowDenoiserE::DEPTH);
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(depth_idx);
				d3d12::CreateSRVFromDSV(data.m_depth_buffer, cpu_handle);
			}

			if (kernel_texture_location.has_value())
			{
				data.m_kernel = texture_pool->Load(kernel_texture_location.value(), false, false);
			}
			else
			{
				int width = 48;
				int height = 48;
				std::vector<unsigned char> kernel_data(width * height * 4);

				for (int i = 0; i < height; ++i)
				{
					for (int j = 0; j < width; ++j)
					{
						kernel_data[(i*width + j) * 4 + 0] =
							kernel_data[(i*width + j) * 4 + 1] =
							kernel_data[(i*width + j) * 4 + 2] =
							kernel_data[(i*width + j) * 4 + 3] =
							static_cast<unsigned char>(CalculateGaussian(
								1.f, 0.f, 5.f,
								sqrtf((i - height / 2.f)*(i - height / 2.f) + (j - width / 2.f) * (j - width / 2.f)))*255.f);

					}
				}

				data.m_kernel = texture_pool->LoadFromMemory(kernel_data.data(), width, height, TextureType::RAW, false, false);
			}

			data.m_constant_buffer_pool = n_render_system.CreateConstantBufferPool(SizeAlignTwoPower(sizeof(temp::ShadowDenoiserSettings_CBData), 256)*d3d12::settings::num_back_buffers);
			data.m_denoiser_settings_buffer_horizontal = data.m_constant_buffer_pool->Create(sizeof(temp::ShadowDenoiserSettings_CBData));
			data.m_denoiser_settings_buffer_vertical = data.m_constant_buffer_pool->Create(sizeof(temp::ShadowDenoiserSettings_CBData));

			data.m_denoiser_settings = {};
			data.m_denoiser_settings.m_depth_contrast = 4.f;

		}

		inline void ExecuteShadowDenoiserTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<ShadowDenoiserData>(handle);
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
			auto render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);

			// Output UAV
			/*{
				auto rtv_out_uav_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::shadow_denoiser, params::ShadowDenoiserE::DEST)));
				std::vector<Format> formats = { render_target->m_create_info.m_rtv_formats[0] };
				d3d12::CreateUAVFromRTV(render_target, rtv_out_uav_handle, 1, formats.data());
			}*/

			// Update camera constant buffer pool
			auto active_camera = sg.GetActiveCamera();

			temp::ProjectionView_CBData camera_data;
			camera_data.m_projection = active_camera->m_projection;
			camera_data.m_inverse_projection = active_camera->m_inverse_projection;
			camera_data.m_view = active_camera->m_view;
			camera_data.m_inverse_view = active_camera->m_inverse_view;
			// Set 'is hybrid' to either 1 or 0 depending on the current frame_graph
			camera_data.m_is_hybrid = true;

			active_camera->m_camera_cb->m_pool->Update(active_camera->m_camera_cb, sizeof(temp::ProjectionView_CBData), 0, (uint8_t*)&camera_data);
			const auto camera_cb = static_cast<D3D12ConstantBufferHandle*>(active_camera->m_camera_cb);

			data.m_denoiser_settings.m_direction = { 1.f, 0.f };
			data.m_constant_buffer_pool->Update(data.m_denoiser_settings_buffer_horizontal, sizeof(temp::ShadowDenoiserSettings_CBData), 0, (uint8_t*)&data.m_denoiser_settings);
			const auto settings_cb_hor = static_cast<D3D12ConstantBufferHandle*>(data.m_denoiser_settings_buffer_horizontal);

			data.m_denoiser_settings.m_direction = { 0.f, 1.f };
			data.m_constant_buffer_pool->Update(data.m_denoiser_settings_buffer_vertical, sizeof(temp::ShadowDenoiserSettings_CBData), 0, (uint8_t*)&data.m_denoiser_settings);
			const auto settings_cb_ver = static_cast<D3D12ConstantBufferHandle*>(data.m_denoiser_settings_buffer_vertical);

			if (n_render_system.m_render_window.has_value())
			{
				const auto viewport = n_render_system.m_viewport;
				const auto frame_idx = n_render_system.GetFrameIdx();

				d3d12::TransitionDepth(cmd_list, data.m_depth_buffer, ResourceState::DEPTH_WRITE, ResourceState::NON_PIXEL_SHADER_RESOURCE);
				
				d3d12::Transition(cmd_list, render_target, ResourceState::COPY_SOURCE, ResourceState::UNORDERED_ACCESS);

				d3d12::BindComputePipeline(cmd_list, data.m_pipeline->m_native);

				d3d12::BindViewport(cmd_list, viewport);

				bool is_fallback = d3d12::GetRaytracingType(n_render_system.m_device) == RaytracingType::FALLBACK;
				d3d12::BindDescriptorHeaps(cmd_list, frame_idx, is_fallback);

				d3d12::BindComputeConstantBuffer(cmd_list, camera_cb->m_native, 1, frame_idx);
				d3d12::BindComputeConstantBuffer(cmd_list, settings_cb_hor->m_native, 2, frame_idx);

				constexpr unsigned int source = rs_layout::GetHeapLoc(params::shadow_denoiser, params::ShadowDenoiserE::SOURCE);
				d3d12::DescHeapCPUHandle source_handle = data.out_allocation.GetDescriptorHandle(source);
				d3d12::SetShaderSRV(cmd_list, 0, source, source_handle);

				constexpr unsigned int dest = rs_layout::GetHeapLoc(params::shadow_denoiser, params::ShadowDenoiserE::DEST);
				d3d12::DescHeapCPUHandle dest_handle = data.out_allocation.GetDescriptorHandle(dest);
				d3d12::SetShaderUAV(cmd_list, 0, dest, dest_handle);

				if (data.m_has_depth_buffer)
				{
					constexpr unsigned int depth = rs_layout::GetHeapLoc(params::shadow_denoiser, params::ShadowDenoiserE::DEPTH);
					d3d12::DescHeapCPUHandle depth_handle = data.out_allocation.GetDescriptorHandle(depth);
					d3d12::SetShaderSRV(cmd_list, 0, depth, depth_handle);
				}

				// Kernel
				{
					constexpr unsigned int kernel_idx = rs_layout::GetHeapLoc(params::shadow_denoiser, params::ShadowDenoiserE::KERNEL);
					auto cpu_handle = data.out_allocation.GetDescriptorHandle(kernel_idx);
					d3d12::CreateSRVFromTexture(static_cast<D3D12TexturePool*>(data.m_kernel.m_pool)->GetTexture(data.m_kernel.m_id), cpu_handle);

					d3d12::SetShaderSRV(cmd_list, 0, kernel_idx, cpu_handle);
				}

				d3d12::Dispatch(cmd_list,
					static_cast<int>(std::ceil(n_render_system.m_viewport.m_viewport.Width / 16.f)),
					static_cast<int>(std::ceil(n_render_system.m_viewport.m_viewport.Height / 16.f)),
					1);

				d3d12::Transition(cmd_list, render_target, ResourceState::UNORDERED_ACCESS, ResourceState::COPY_SOURCE);
				d3d12::Transition(cmd_list, data.m_noisy_buffer, ResourceState::COPY_SOURCE, ResourceState::COPY_DEST);

				cmd_list->m_native->CopyResource(data.m_noisy_buffer->m_render_targets[0], render_target->m_render_targets[0]);
				
				d3d12::Transition(cmd_list, render_target, ResourceState::COPY_SOURCE, ResourceState::UNORDERED_ACCESS);
				d3d12::Transition(cmd_list, data.m_noisy_buffer, ResourceState::COPY_DEST, ResourceState::COPY_SOURCE);
				
				d3d12::BindComputeConstantBuffer(cmd_list, settings_cb_ver->m_native, 2, frame_idx);

				d3d12::Dispatch(cmd_list,
					static_cast<int>(std::ceil(n_render_system.m_viewport.m_viewport.Width / 16.f)),
					static_cast<int>(std::ceil(n_render_system.m_viewport.m_viewport.Height / 16.f)),
					1);

				d3d12::Transition(cmd_list, render_target, ResourceState::UNORDERED_ACCESS, ResourceState::COPY_SOURCE);

				d3d12::TransitionDepth(cmd_list, data.m_depth_buffer, ResourceState::NON_PIXEL_SHADER_RESOURCE, ResourceState::DEPTH_WRITE);
			}
		}

	} /* internal */

	// pass a path to a texture location to load a custom denoiser kernel
	inline void AddShadowDenoiserTask(FrameGraph& fg, std::optional<std::string_view> kernel_texture_location)
	{
		std::wstring name(L"Shadow Denoiser");

		RenderTargetProperties rt_properties
		{
			RenderTargetProperties::IsRenderWindow(false),
			RenderTargetProperties::Width(std::nullopt),
			RenderTargetProperties::Height(std::nullopt),
			RenderTargetProperties::ExecuteResourceState(ResourceState::UNORDERED_ACCESS),
			RenderTargetProperties::FinishedResourceState(ResourceState::COPY_SOURCE),
			RenderTargetProperties::CreateDSVBuffer(false),
			RenderTargetProperties::DSVFormat(Format::UNKNOWN),
			RenderTargetProperties::RTVFormats({ Format::R32G32B32A32_FLOAT }),
			RenderTargetProperties::NumRTVFormats(1),
			RenderTargetProperties::Clear(true),
			RenderTargetProperties::ClearDepth(true),
			RenderTargetProperties::ResourceName(name)
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [kernel_texture_location](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			internal::SetupShadowDenoiserTask(rs, fg, handle, resize, kernel_texture_location);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle)
		{
			internal::ExecuteShadowDenoiserTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			
		};

		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		fg.AddTask<ShadowDenoiserData>(desc);
	}

}/* wr */
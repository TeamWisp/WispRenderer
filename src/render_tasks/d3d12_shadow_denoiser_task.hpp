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
		d3d12::RenderTarget* m_variance_in_buffer;
		d3d12::RenderTarget* m_variance_out_buffer;

		d3d12::RenderTarget* m_accum_buffer;
		
		wr::D3D12Pipeline* m_pipeline;
		wr::D3D12Pipeline* m_accum_pipeline;

		DescriptorAllocator* out_allocator;
		DescriptorAllocation out_allocation;

		std::shared_ptr<ConstantBufferPool> m_constant_buffer_pool;

		temp::ShadowDenoiserSettings_CBData m_denoiser_settings;
		ConstantBufferHandle* m_denoiser_settings_buffer_horizontal;
		ConstantBufferHandle* m_denoiser_settings_buffer_vertical;

		ConstantBufferHandle* m_denoiser_camera;

		TextureHandle m_kernel;

		DirectX::XMMATRIX m_prev_view;
		DirectX::XMMATRIX m_prev_projection;
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
			data.m_accum_pipeline = (D3D12Pipeline*)ps_registry.Find(pipelines::temporal_accumulator);

			//Retrieve the texture pool from the render system. It will be used to allocate temporary cpu visible descriptors
			std::shared_ptr<D3D12TexturePool> texture_pool = std::static_pointer_cast<D3D12TexturePool>(n_render_system.m_texture_pools[0]);
			if (!texture_pool)
			{
				LOGC("Texture pool is nullptr. This shouldn't happen as the render system should always create the first texture pool");
			}

			data.out_allocator = texture_pool->GetAllocator(DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV);
			data.out_allocation = std::move(data.out_allocator->Allocate(7));

			data.m_noisy_buffer = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<wr::RTShadowData>());
			data.m_output_buffer = fg.GetRenderTarget<d3d12::RenderTarget>(handle);

			d3d12::desc::RenderTargetDesc accum_buffer_desc = {};
			accum_buffer_desc.m_clear_color[0] = 1.f;
			accum_buffer_desc.m_clear_color[1] = 1.f;
			accum_buffer_desc.m_clear_color[2] = 1.f;
			accum_buffer_desc.m_clear_color[3] = 0.f;
			accum_buffer_desc.m_create_dsv_buffer = false;
			accum_buffer_desc.m_dsv_format = Format::UNKNOWN;
			accum_buffer_desc.m_initial_state = ResourceState::NON_PIXEL_SHADER_RESOURCE;
			accum_buffer_desc.m_num_rtv_formats = 1;
			accum_buffer_desc.m_rtv_formats[0] = Format::R32G32B32A32_FLOAT;

			data.m_accum_buffer = d3d12::CreateRenderTarget(
				n_render_system.m_device,
				n_render_system.m_viewport.m_viewport.Width,
				n_render_system.m_viewport.m_viewport.Height,
				accum_buffer_desc
			);

			d3d12::desc::RenderTargetDesc variance_buffer_desc = {};
			variance_buffer_desc.m_clear_color[0] = 0.f;
			variance_buffer_desc.m_create_dsv_buffer = false;
			variance_buffer_desc.m_dsv_format = Format::UNKNOWN;
			variance_buffer_desc.m_initial_state = ResourceState::UNORDERED_ACCESS;
			variance_buffer_desc.m_num_rtv_formats = 1;
			variance_buffer_desc.m_rtv_formats[0] = Format::R32G32B32A32_FLOAT;

			data.m_variance_out_buffer = d3d12::CreateRenderTarget(
				n_render_system.m_device,
				n_render_system.m_viewport.m_viewport.Width,
				n_render_system.m_viewport.m_viewport.Height,
				variance_buffer_desc
			);

			variance_buffer_desc.m_initial_state = ResourceState::NON_PIXEL_SHADER_RESOURCE;

			data.m_variance_in_buffer = d3d12::CreateRenderTarget(
				n_render_system.m_device,
				n_render_system.m_viewport.m_viewport.Width,
				n_render_system.m_viewport.m_viewport.Height,
				variance_buffer_desc
			);

			if (fg.HasTask<wr::DeferredMainTaskData>())
			{
				data.m_has_depth_buffer = true;
				data.m_depth_buffer = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<wr::DeferredMainTaskData>());
			}

			{
				constexpr unsigned int source_idx = rs_layout::GetHeapLoc(params::shadow_denoiser, params::ShadowDenoiserE::SOURCE);
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(source_idx);
				d3d12::CreateSRVFromRTV(data.m_noisy_buffer, cpu_handle, 1, data.m_noisy_buffer->m_create_info.m_rtv_formats.data());
			}

			if(data.m_has_depth_buffer)
			{
				constexpr unsigned int depth_idx = rs_layout::GetHeapLoc(params::shadow_denoiser, params::ShadowDenoiserE::DEPTH);
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(depth_idx);
				d3d12::CreateSRVFromDSV(data.m_depth_buffer, cpu_handle);
			}

			{
				constexpr unsigned int accum_idx = rs_layout::GetHeapLoc(params::shadow_denoiser, params::ShadowDenoiserE::ACCUM);
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(accum_idx);
				d3d12::CreateSRVFromRTV(data.m_accum_buffer, cpu_handle, 1, data.m_accum_buffer->m_create_info.m_rtv_formats.data());
			}

			{
				constexpr unsigned int var_idx = rs_layout::GetHeapLoc(params::shadow_denoiser, params::ShadowDenoiserE::VARIANCE_IN);
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(var_idx);
				d3d12::CreateSRVFromRTV(data.m_variance_in_buffer, cpu_handle, 1, data.m_variance_in_buffer->m_create_info.m_rtv_formats.data());
			}

			{
				constexpr unsigned int out_idx = rs_layout::GetHeapLoc(params::shadow_denoiser, params::ShadowDenoiserE::DEST);
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(out_idx);
				d3d12::CreateUAVFromRTV(data.m_output_buffer, cpu_handle, 1, data.m_output_buffer->m_create_info.m_rtv_formats.data());
			}

			{
				constexpr unsigned int var_idx = rs_layout::GetHeapLoc(params::shadow_denoiser, params::ShadowDenoiserE::VARIANCE_OUT);
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(var_idx);
				d3d12::CreateUAVFromRTV(data.m_variance_out_buffer, cpu_handle, 1, data.m_variance_out_buffer->m_create_info.m_rtv_formats.data());
			}

			if (kernel_texture_location.has_value())
			{
				data.m_kernel = texture_pool->Load(kernel_texture_location.value(), false, false);
			}
			else
			{
				int width = 127;
				int height = 127;
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
								1.f, 0.f, 16.f,
								sqrtf((i - height / 2.f)*(i - height / 2.f) + (j - width / 2.f) * (j - width / 2.f)))*255.f);

					}
				}

				data.m_kernel = texture_pool->LoadFromMemory(kernel_data.data(), width, height, TextureType::RAW, false, false);
			}

			data.m_constant_buffer_pool = n_render_system.CreateConstantBufferPool(
				SizeAlignTwoPower(sizeof(temp::ShadowDenoiserSettings_CBData), 256)*d3d12::settings::num_back_buffers * 2 +
				SizeAlignTwoPower(sizeof(temp::DenoiserCamera_CBData), 256)*d3d12::settings::num_back_buffers);
			data.m_denoiser_settings_buffer_horizontal = data.m_constant_buffer_pool->Create(sizeof(temp::ShadowDenoiserSettings_CBData));
			data.m_denoiser_settings_buffer_vertical = data.m_constant_buffer_pool->Create(sizeof(temp::ShadowDenoiserSettings_CBData));
			data.m_denoiser_camera = data.m_constant_buffer_pool->Create(sizeof(temp::DenoiserCamera_CBData));

			data.m_denoiser_settings = {};
			data.m_denoiser_settings.m_depth_contrast = 4.f;
			data.m_denoiser_settings.m_kernel_size = { 5, 5 };

		}

		inline void BindResources(D3D12RenderSystem& n_render_system, d3d12::CommandList* cmd_list, ShadowDenoiserData& data, bool is_fallback)
		{
			d3d12::BindDescriptorHeaps(cmd_list, n_render_system.GetFrameIdx(), is_fallback);

			{
				constexpr unsigned int source = rs_layout::GetHeapLoc(params::shadow_denoiser, params::ShadowDenoiserE::SOURCE);
				d3d12::DescHeapCPUHandle source_handle = data.out_allocation.GetDescriptorHandle(source);
				d3d12::SetShaderSRV(cmd_list, 0, source, source_handle);
			}

			{
				constexpr unsigned int depth = rs_layout::GetHeapLoc(params::shadow_denoiser, params::ShadowDenoiserE::DEPTH);
				d3d12::DescHeapCPUHandle depth_handle = data.out_allocation.GetDescriptorHandle(depth);
				d3d12::SetShaderSRV(cmd_list, 0, depth, depth_handle);
			}

			{
				constexpr unsigned int kernel = rs_layout::GetHeapLoc(params::shadow_denoiser, params::ShadowDenoiserE::KERNEL);
				d3d12::DescHeapCPUHandle kernel_handle = data.out_allocation.GetDescriptorHandle(kernel);
				d3d12::CreateSRVFromTexture(static_cast<D3D12TexturePool*>(data.m_kernel.m_pool)->GetTexture(data.m_kernel.m_id), kernel_handle);
				d3d12::SetShaderSRV(cmd_list, 0, kernel, kernel_handle);
			}

			{
				constexpr unsigned int accum = rs_layout::GetHeapLoc(params::shadow_denoiser, params::ShadowDenoiserE::ACCUM);
				d3d12::DescHeapCPUHandle accum_handle = data.out_allocation.GetDescriptorHandle(accum);
				d3d12::SetShaderSRV(cmd_list, 0, accum, accum_handle);
			}

			{
				constexpr unsigned int var = rs_layout::GetHeapLoc(params::shadow_denoiser, params::ShadowDenoiserE::VARIANCE_IN);
				d3d12::DescHeapCPUHandle var_handle = data.out_allocation.GetDescriptorHandle(var);
				d3d12::SetShaderSRV(cmd_list, 0, var, var_handle);
			}

			{
				constexpr unsigned int dest = rs_layout::GetHeapLoc(params::shadow_denoiser, params::ShadowDenoiserE::DEST);
				d3d12::DescHeapCPUHandle dest_handle = data.out_allocation.GetDescriptorHandle(dest);
				d3d12::SetShaderUAV(cmd_list, 0, dest, dest_handle);
			}

			{
				constexpr unsigned int variance = rs_layout::GetHeapLoc(params::shadow_denoiser, params::ShadowDenoiserE::VARIANCE_OUT);
				d3d12::DescHeapCPUHandle variance_handle = data.out_allocation.GetDescriptorHandle(variance);
				d3d12::SetShaderUAV(cmd_list, 0, variance, variance_handle);
			}
		}

		inline void Accumulate(D3D12RenderSystem& n_render_system, d3d12::CommandList* cmd_list, ShadowDenoiserData& data, bool is_fallback)
		{
			const auto camera_cb = static_cast<D3D12ConstantBufferHandle*>(data.m_denoiser_camera);
			const auto settings_cb_hor = static_cast<D3D12ConstantBufferHandle*>(data.m_denoiser_settings_buffer_horizontal);
			const auto settings_cb_ver = static_cast<D3D12ConstantBufferHandle*>(data.m_denoiser_settings_buffer_vertical);

			// Prefilter accumulation
			{
				d3d12::BindComputePipeline(cmd_list, data.m_accum_pipeline->m_native);

				d3d12::BindComputeConstantBuffer(cmd_list, camera_cb->m_native, 1, n_render_system.GetFrameIdx());
				d3d12::BindComputeConstantBuffer(cmd_list, settings_cb_hor->m_native, 2, n_render_system.GetFrameIdx());

				BindResources(n_render_system, cmd_list, data, is_fallback);

				d3d12::Dispatch(cmd_list,
					static_cast<int>(std::ceil(n_render_system.m_viewport.m_viewport.Width / 16.f)),
					static_cast<int>(std::ceil(n_render_system.m_viewport.m_viewport.Height / 16.f)),
					1);
			}

			D3D12_RESOURCE_BARRIER barriers[] = {
				CD3DX12_RESOURCE_BARRIER::UAV(data.m_output_buffer->m_render_targets[0]),
				CD3DX12_RESOURCE_BARRIER::UAV(data.m_variance_out_buffer->m_render_targets[0])
			};

			cmd_list->m_native->ResourceBarrier(2, barriers);

			// Storing accumulation data
			{
				d3d12::Transition(cmd_list, data.m_output_buffer, ResourceState::UNORDERED_ACCESS, ResourceState::COPY_SOURCE);
				d3d12::Transition(cmd_list, data.m_accum_buffer, ResourceState::NON_PIXEL_SHADER_RESOURCE, ResourceState::COPY_DEST);
				d3d12::Transition(cmd_list, data.m_noisy_buffer, ResourceState::COPY_SOURCE, ResourceState::COPY_DEST);
				d3d12::Transition(cmd_list, data.m_variance_out_buffer, ResourceState::UNORDERED_ACCESS, ResourceState::COPY_SOURCE);
				d3d12::Transition(cmd_list, data.m_variance_in_buffer, ResourceState::NON_PIXEL_SHADER_RESOURCE, ResourceState::COPY_DEST);

				cmd_list->m_native->CopyResource(data.m_accum_buffer->m_render_targets[0], data.m_output_buffer->m_render_targets[0]);
				cmd_list->m_native->CopyResource(data.m_noisy_buffer->m_render_targets[0], data.m_output_buffer->m_render_targets[0]);
				cmd_list->m_native->CopyResource(data.m_variance_in_buffer->m_render_targets[0], data.m_variance_out_buffer->m_render_targets[0]);

				d3d12::Transition(cmd_list, data.m_output_buffer, ResourceState::COPY_SOURCE, ResourceState::UNORDERED_ACCESS);
				d3d12::Transition(cmd_list, data.m_accum_buffer, ResourceState::COPY_DEST, ResourceState::NON_PIXEL_SHADER_RESOURCE);
				d3d12::Transition(cmd_list, data.m_noisy_buffer, ResourceState::COPY_DEST, ResourceState::COPY_SOURCE);
				d3d12::Transition(cmd_list, data.m_variance_out_buffer, ResourceState::COPY_SOURCE, ResourceState::UNORDERED_ACCESS);
				d3d12::Transition(cmd_list, data.m_variance_in_buffer, ResourceState::COPY_DEST, ResourceState::NON_PIXEL_SHADER_RESOURCE);
			}
		}

		inline void Blur(D3D12RenderSystem& n_render_system, SceneGraph& sg, d3d12::CommandList* cmd_list, ShadowDenoiserData& data, bool is_fallback)
		{
			unsigned int frame_idx = n_render_system.GetFrameIdx();
			
			const auto camera_cb = static_cast<D3D12ConstantBufferHandle*>(data.m_denoiser_camera);
			const auto settings_cb_hor = static_cast<D3D12ConstantBufferHandle*>(data.m_denoiser_settings_buffer_horizontal);
			const auto settings_cb_ver = static_cast<D3D12ConstantBufferHandle*>(data.m_denoiser_settings_buffer_vertical);

			// 2-pass denoising
			{
				d3d12::BindComputePipeline(cmd_list, data.m_pipeline->m_native);

				d3d12::BindViewport(cmd_list, n_render_system.m_viewport);

				BindResources(n_render_system, cmd_list, data, is_fallback);

				d3d12::BindComputeConstantBuffer(cmd_list, camera_cb->m_native, 1, frame_idx);
				d3d12::BindComputeConstantBuffer(cmd_list, settings_cb_hor->m_native, 2, frame_idx);

				d3d12::Dispatch(cmd_list,
					static_cast<int>(std::ceil(n_render_system.m_viewport.m_viewport.Width / 16.f)),
					static_cast<int>(std::ceil(n_render_system.m_viewport.m_viewport.Height / 16.f)),
					1);

				/*d3d12::Transition(cmd_list, data.m_output_buffer, ResourceState::UNORDERED_ACCESS, ResourceState::COPY_SOURCE);
				d3d12::Transition(cmd_list, data.m_noisy_buffer, ResourceState::COPY_SOURCE, ResourceState::COPY_DEST);

				cmd_list->m_native->CopyResource(data.m_noisy_buffer->m_render_targets[0], data.m_output_buffer->m_render_targets[0]);

				d3d12::Transition(cmd_list, data.m_output_buffer, ResourceState::COPY_SOURCE, ResourceState::UNORDERED_ACCESS);
				d3d12::Transition(cmd_list, data.m_noisy_buffer, ResourceState::COPY_DEST, ResourceState::COPY_SOURCE);

				d3d12::BindComputeConstantBuffer(cmd_list, settings_cb_ver->m_native, 2, frame_idx);

				d3d12::Dispatch(cmd_list,
					static_cast<int>(std::ceil(n_render_system.m_viewport.m_viewport.Width / 16.f)),
					static_cast<int>(std::ceil(n_render_system.m_viewport.m_viewport.Height / 16.f)),
					1);*/

				cmd_list->m_native->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(data.m_output_buffer->m_render_targets[0]));

				d3d12::Transition(cmd_list, data.m_output_buffer, ResourceState::UNORDERED_ACCESS, ResourceState::COPY_SOURCE);
				d3d12::Transition(cmd_list, data.m_noisy_buffer, ResourceState::COPY_SOURCE, ResourceState::COPY_DEST);

				cmd_list->m_native->CopyResource(data.m_noisy_buffer->m_render_targets[0], data.m_output_buffer->m_render_targets[0]);

				d3d12::Transition(cmd_list, data.m_output_buffer, ResourceState::COPY_SOURCE, ResourceState::UNORDERED_ACCESS);
				d3d12::Transition(cmd_list, data.m_noisy_buffer, ResourceState::COPY_DEST, ResourceState::COPY_SOURCE);
			}
		}

		inline void ExecuteShadowDenoiserTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<ShadowDenoiserData>(handle);
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
			auto render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);
			bool is_fallback = d3d12::GetRaytracingType(n_render_system.m_device) == RaytracingType::FALLBACK;

			// Output UAV
			/*{
				auto rtv_out_uav_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::shadow_denoiser, params::ShadowDenoiserE::DEST)));
				std::vector<Format> formats = { render_target->m_create_info.m_rtv_formats[0] };
				d3d12::CreateUAVFromRTV(render_target, rtv_out_uav_handle, 1, formats.data());
			}*/
			auto active_camera = sg.GetActiveCamera();

			temp::DenoiserCamera_CBData camera_data;
			camera_data.m_projection = active_camera->m_projection;
			camera_data.m_inverse_projection = active_camera->m_inverse_projection;
			camera_data.m_view = active_camera->m_view;
			camera_data.m_inverse_view = active_camera->m_inverse_view;
			camera_data.m_prev_projection = data.m_prev_projection;
			camera_data.m_prev_view = data.m_prev_view;

			data.m_constant_buffer_pool->Update(data.m_denoiser_camera, sizeof(temp::DenoiserCamera_CBData), 0, (uint8_t*)&camera_data);
			const auto camera_cb = static_cast<D3D12ConstantBufferHandle*>(data.m_denoiser_camera);

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
				
				Accumulate(n_render_system, cmd_list, data, is_fallback);

				Blur(n_render_system, sg, cmd_list, data, is_fallback);

				//Accumulate(n_render_system, cmd_list, data, is_fallback);

				//Blur(n_render_system, sg, cmd_list, data, is_fallback);
				
				d3d12::Transition(cmd_list, render_target, ResourceState::UNORDERED_ACCESS, ResourceState::COPY_SOURCE);

				d3d12::TransitionDepth(cmd_list, data.m_depth_buffer, ResourceState::NON_PIXEL_SHADER_RESOURCE, ResourceState::DEPTH_WRITE);

				cmd_list->m_native->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(data.m_output_buffer->m_render_targets[0]));
			}

			data.m_prev_view = active_camera->m_view;
			data.m_prev_projection = active_camera->m_projection;
		}

		inline void DestroyShadowDenoiserTask(FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			auto& data = fg.GetData<ShadowDenoiserData>(handle);

			d3d12::Destroy(data.m_accum_buffer);
			d3d12::Destroy(data.m_variance_in_buffer);
			d3d12::Destroy(data.m_variance_out_buffer);

			DescriptorAllocation temp1 = std::move(data.out_allocation);
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
			internal::DestroyShadowDenoiserTask(fg, handle, resize);
		};

		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		fg.AddTask<ShadowDenoiserData>(desc);
	}

}/* wr */
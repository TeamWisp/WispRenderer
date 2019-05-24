#pragma once
#include "../render_tasks/d3d12_deferred_composition.hpp"

#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "../d3d12/d3d12_structured_buffer_pool.hpp"
#include "../scene_graph/camera_node.hpp"
#include "../scene_graph/skybox_node.hpp"
#include "../engine_registry.hpp"

#include "../render_tasks/d3d12_brdf_lut_precalculation.hpp"
#include "../render_tasks/d3d12_deferred_main.hpp"
#include "../render_tasks/d3d12_spatial_reconstruction.hpp"
#include "../render_tasks/d3d12_cubemap_convolution.hpp"
#include "../render_tasks/d3d12_rt_hybrid_task.hpp"
#include "../render_tasks/d3d12_path_tracer.hpp"
#include "../render_tasks/d3d12_accumulation.hpp"
#include "../render_tasks/d3d12_rtao_task.hpp"
#include "d3d12_hbao.hpp"

namespace wr
{
	namespace internal
	{

		void RecordDrawCommands(D3D12RenderSystem& render_system, d3d12::CommandList* cmd_list, d3d12::HeapResource* camera_cb, wr::DeferredCompositionTaskData const& data, unsigned int frame_idx)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);

			d3d12::BindComputePipeline(cmd_list, data.in_pipeline->m_native);

			bool is_fallback = d3d12::GetRaytracingType(render_system.m_device) == RaytracingType::FALLBACK;
			d3d12::BindDescriptorHeaps(cmd_list, is_fallback);

			d3d12::BindComputeConstantBuffer(cmd_list, camera_cb, 0, frame_idx);

			constexpr unsigned int albedo_loc = rs_layout::GetHeapLoc(params::deferred_composition, params::DeferredCompositionE::GBUFFER_ALBEDO_ROUGHNESS);
			d3d12::DescHeapCPUHandle albedo_handle = data.out_gbuffer_albedo_alloc.GetDescriptorHandle(frame_idx);
			d3d12::SetShaderSRV(cmd_list, 1, albedo_loc, albedo_handle);

			constexpr unsigned int normal_loc = rs_layout::GetHeapLoc(params::deferred_composition, params::DeferredCompositionE::GBUFFER_NORMAL_METALLIC);
			d3d12::DescHeapCPUHandle normal_handle = data.out_gbuffer_normal_alloc.GetDescriptorHandle(frame_idx);
			d3d12::SetShaderSRV(cmd_list, 1, normal_loc, normal_handle);

			constexpr unsigned int emissive_loc = rs_layout::GetHeapLoc(params::deferred_composition, params::DeferredCompositionE::GBUFFER_EMISSIVE_AO);
			d3d12::DescHeapCPUHandle emissive_handle = data.out_gbuffer_emissive_alloc.GetDescriptorHandle(frame_idx);
			d3d12::SetShaderSRV(cmd_list, 1, emissive_loc, emissive_handle);

			constexpr unsigned int depth_loc = rs_layout::GetHeapLoc(params::deferred_composition, params::DeferredCompositionE::GBUFFER_DEPTH);
			d3d12::DescHeapCPUHandle depth_handle = data.out_gbuffer_depth_alloc.GetDescriptorHandle();
			d3d12::SetShaderSRV(cmd_list, 1, depth_loc, depth_handle);

			constexpr unsigned int lights_loc = rs_layout::GetHeapLoc(params::deferred_composition, params::DeferredCompositionE::LIGHT_BUFFER);
			d3d12::DescHeapCPUHandle lights_handle = data.out_lights_alloc.GetDescriptorHandle();
			d3d12::SetShaderSRV(cmd_list, 1, lights_loc, lights_handle);

			constexpr unsigned int skybox = rs_layout::GetHeapLoc(params::deferred_composition, params::DeferredCompositionE::SKY_BOX);
			d3d12::SetShaderSRV(cmd_list, 1, skybox, data.out_skybox);

			constexpr unsigned int irradiance = rs_layout::GetHeapLoc(params::deferred_composition, params::DeferredCompositionE::IRRADIANCE_MAP);
			d3d12::SetShaderSRV(cmd_list, 1, irradiance, data.out_irradiance);

			constexpr unsigned int pref_env = rs_layout::GetHeapLoc(params::deferred_composition, params::DeferredCompositionE::PREF_ENV_MAP);
			d3d12::SetShaderSRV(cmd_list, 1, pref_env, data.out_pref_env_map);

			constexpr unsigned int brdf_lut_loc = rs_layout::GetHeapLoc(params::deferred_composition, params::DeferredCompositionE::BRDF_LUT);
			auto* brdf_lut_text = static_cast<d3d12::TextureResource*>(n_render_system.m_brdf_lut.value().m_pool->GetTextureResource(n_render_system.m_brdf_lut.value()));
			d3d12::SetShaderSRV(cmd_list, 1, brdf_lut_loc, brdf_lut_text);

			constexpr unsigned int shadow_loc = rs_layout::GetHeapLoc(params::deferred_composition, params::DeferredCompositionE::BUFFER_REFLECTION_SHADOW);
			d3d12::DescHeapCPUHandle shadow_handle = data.out_buffer_refl_shadow_alloc.GetDescriptorHandle();
			d3d12::SetShaderSRV(cmd_list, 1, shadow_loc, shadow_handle);
			shadow_handle = data.out_buffer_refl_shadow_alloc.GetDescriptorHandle(1);
			d3d12::SetShaderSRV(cmd_list, 1, shadow_loc + 1, shadow_handle);

			constexpr unsigned int sp_irradiance_loc = rs_layout::GetHeapLoc(params::deferred_composition, params::DeferredCompositionE::BUFFER_SCREEN_SPACE_IRRADIANCE);
			d3d12::DescHeapCPUHandle sp_irradiance_handle = data.out_screen_space_irradiance_alloc.GetDescriptorHandle();
			d3d12::SetShaderSRV(cmd_list, 1, sp_irradiance_loc, sp_irradiance_handle);

			constexpr unsigned int ao_loc = rs_layout::GetHeapLoc(params::deferred_composition, params::DeferredCompositionE::BUFFER_AO);
			d3d12::DescHeapCPUHandle ao_handle = data.out_screen_space_ao_alloc.GetDescriptorHandle();
			d3d12::SetShaderSRV(cmd_list, 1, ao_loc, ao_handle);

			constexpr unsigned int output_loc = rs_layout::GetHeapLoc(params::deferred_composition, params::DeferredCompositionE::OUTPUT);
			d3d12::DescHeapCPUHandle output_handle = data.out_output_alloc.GetDescriptorHandle();
			d3d12::SetShaderUAV(cmd_list, 1, output_loc, output_handle);


			d3d12::Dispatch(cmd_list,
				static_cast<int>(std::ceil(render_system.m_viewport.m_viewport.Width / 16.f)),
				static_cast<int>(std::ceil(render_system.m_viewport.m_viewport.Height / 16.f)),
				1);
		}

		void SetupDeferredCompositionTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<DeferredCompositionTaskData>(handle);

			auto& ps_registry = PipelineRegistry::Get();
			data.in_pipeline = (D3D12Pipeline*)ps_registry.Find(pipelines::deferred_composition);

			// Check if the current frame graph contains the hybrid task to know if it is hybrid or not.
			data.is_hybrid = fg.HasTask<wr::RTHybridData>();
			data.is_path_tracer = fg.HasTask<wr::PathTracerData>();
			data.is_rtao = fg.HasTask<wr::RTAOData>();
			data.is_hbao = fg.HasTask<wr::HBAOData>() && !data.is_rtao; //Don't use HBAO when RTAO is active

			//Retrieve the texture pool from the render system. It will be used to allocate temporary cpu visible descriptors
			std::shared_ptr<D3D12TexturePool> texture_pool = std::static_pointer_cast<D3D12TexturePool>(n_render_system.m_texture_pools[0]);
			if (!texture_pool)
			{
				LOGC("Texture pool is nullptr. This shouldn't happen as the render system should always create the first texture pool");
			}

			if (!resize)
			{
				data.out_allocator = texture_pool->GetAllocator(DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV);

				data.out_gbuffer_albedo_alloc = std::move(data.out_allocator->Allocate(d3d12::settings::num_back_buffers));
				data.out_gbuffer_normal_alloc = std::move(data.out_allocator->Allocate(d3d12::settings::num_back_buffers));
				data.out_gbuffer_emissive_alloc = std::move(data.out_allocator->Allocate(d3d12::settings::num_back_buffers));
				data.out_gbuffer_depth_alloc = std::move(data.out_allocator->Allocate());
				data.out_lights_alloc = std::move(data.out_allocator->Allocate());
				data.out_buffer_refl_shadow_alloc = std::move(data.out_allocator->Allocate(2));
				data.out_screen_space_irradiance_alloc = std::move(data.out_allocator->Allocate());
				data.out_screen_space_ao_alloc = std::move(data.out_allocator->Allocate());
				data.out_output_alloc = std::move(data.out_allocator->Allocate());
			}

			for (uint32_t i = 0; i < d3d12::settings::num_back_buffers; ++i)
			{
				auto albedo_handle = data.out_gbuffer_albedo_alloc.GetDescriptorHandle(i);
				auto normal_handle = data.out_gbuffer_normal_alloc.GetDescriptorHandle(i);
				auto emissive_handle = data.out_gbuffer_emissive_alloc.GetDescriptorHandle(i);
				auto depth_handle = data.out_gbuffer_depth_alloc.GetDescriptorHandle();

				auto deferred_main_rt = data.out_deferred_main_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<DeferredMainTaskData>());

				d3d12::CreateSRVFromSpecificRTV(deferred_main_rt, albedo_handle, 0, deferred_main_rt->m_create_info.m_rtv_formats[0]);
				d3d12::CreateSRVFromSpecificRTV(deferred_main_rt, normal_handle, 1, deferred_main_rt->m_create_info.m_rtv_formats[1]);
				d3d12::CreateSRVFromSpecificRTV(deferred_main_rt, emissive_handle, 2, deferred_main_rt->m_create_info.m_rtv_formats[2]);
				
				d3d12::CreateSRVFromDSV(deferred_main_rt, depth_handle);

				// Bind output(s) from hybrid render task, if the composition task is executed in the hybrid frame graph
				if (data.is_hybrid)
				{
					auto shadow_handle = data.out_buffer_refl_shadow_alloc.GetDescriptorHandle(1);

					auto hybrid_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<wr::RTHybridData>());
					d3d12::CreateSRVFromSpecificRTV(hybrid_rt, shadow_handle, 1, hybrid_rt->m_create_info.m_rtv_formats.data()[1]);

					auto filtered_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<wr::SpatialReconstructionData>());
					shadow_handle = data.out_buffer_refl_shadow_alloc.GetDescriptorHandle();
					d3d12::CreateSRVFromSpecificRTV(hybrid_rt, shadow_handle, 0, filtered_rt->m_create_info.m_rtv_formats.data()[0]);
				
				}			
				if (data.is_rtao)
				{
					auto ao_handle =  data.out_screen_space_ao_alloc.GetDescriptorHandle();

					auto ao_buffer = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<wr::RTAOData>());
					d3d12::CreateSRVFromRTV(ao_buffer, ao_handle, 1, ao_buffer->m_create_info.m_rtv_formats.data());
				} 
			}				
		}

		void ExecuteDeferredCompositionTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& scene_graph, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<DeferredCompositionTaskData>(handle);
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
			auto render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);

			fg.WaitForPredecessorTask<CubemapConvolutionTaskData>();

			if (data.is_hybrid)
			{
				// Wait on hybrid task
				fg.WaitForPredecessorTask<RTHybridData>();
			}

			if (n_render_system.m_render_window.has_value())
			{
				const auto viewport = n_render_system.m_viewport;
				const auto frame_idx = n_render_system.GetFrameIdx();

				// Update camera constant buffer pool
				auto active_camera = scene_graph.GetActiveCamera();

				temp::ProjectionView_CBData camera_data{};
				camera_data.m_projection = active_camera->m_projection;
				camera_data.m_inverse_projection = active_camera->m_inverse_projection;
				camera_data.m_view = active_camera->m_view;
				camera_data.m_inverse_view = active_camera->m_inverse_view;
				camera_data.m_is_hybrid = data.is_hybrid;
				camera_data.m_is_path_tracer = data.is_path_tracer;
				if (data.is_rtao)
				{
					camera_data.m_is_ao = true;
				}
				else
				{
#ifdef NVIDIA_GAMEWORKS_HBAO
					camera_data.m_is_ao = data.is_hbao;
#else
					camera_data.m_is_ao = false;
#endif
				}
				active_camera->m_camera_cb->m_pool->Update(active_camera->m_camera_cb, sizeof(temp::ProjectionView_CBData), 0, (uint8_t*)&camera_data);
				const auto camera_cb = static_cast<D3D12ConstantBufferHandle*>(active_camera->m_camera_cb);

				if (static_cast<D3D12StructuredBufferHandle*>(scene_graph.GetLightBuffer())->m_native->m_states[frame_idx] != ResourceState::NON_PIXEL_SHADER_RESOURCE)
				{
					static_cast<D3D12StructuredBufferPool*>(scene_graph.GetLightBuffer()->m_pool)->SetBufferState(scene_graph.GetLightBuffer(), ResourceState::NON_PIXEL_SHADER_RESOURCE);
					return;
				}

				//Get light buffer
				{
					auto srv_struct_buffer_handle = data.out_lights_alloc.GetDescriptorHandle();
					d3d12::CreateSRVFromStructuredBuffer(static_cast<D3D12StructuredBufferHandle*>(scene_graph.GetLightBuffer())->m_native, srv_struct_buffer_handle, frame_idx);
				}

				//GetSkybox
				auto skybox = scene_graph.GetCurrentSkybox();
				if (skybox != nullptr)
				{
					//data.out_skybox = static_cast<wr::d3d12::TextureResource*>(pred_data.in_radiance.m_pool->GetTexture(pred_data.in_radiance.m_id));
					data.out_skybox = static_cast<wr::d3d12::TextureResource*>(skybox->m_skybox->m_pool->GetTextureResource(skybox->m_skybox.value()));
					d3d12::CreateSRVFromTexture(data.out_skybox);
				}

				// Get Screen Space Environment Texture
				if (data.is_path_tracer)
				{
					auto irradiance_handle = data.out_screen_space_irradiance_alloc.GetDescriptorHandle();
					auto pred_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<wr::AccumulationData>());
					d3d12::CreateSRVFromSpecificRTV(pred_rt, irradiance_handle, 0, pred_rt->m_create_info.m_rtv_formats[0]);
				}

				// Get HBAO+ Texture
				if (data.is_hbao)
				{
					auto handle =  data.out_screen_space_ao_alloc.GetDescriptorHandle();
					auto ao_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<wr::HBAOData>());
					d3d12::CreateSRVFromSpecificRTV(ao_rt, handle, 0, ao_rt->m_create_info.m_rtv_formats[0]);
				}

				// Get Irradiance Map
				if (skybox != nullptr)
				{
					data.out_irradiance = static_cast<wr::d3d12::TextureResource*>(skybox->m_irradiance->m_pool->GetTextureResource(skybox->m_irradiance.value()));
					d3d12::CreateSRVFromTexture(data.out_irradiance);
				}

				// Get the prefiltered environment map	
				if (skybox != nullptr)
				{
					data.out_pref_env_map = static_cast<wr::d3d12::TextureResource*>(skybox->m_prefiltered_env_map->m_pool->GetTextureResource(skybox->m_prefiltered_env_map.value()));
					d3d12::CreateSRVFromTexture(data.out_pref_env_map);
				}

				// Output UAV
				{
					auto rtv_out_uav_handle = data.out_output_alloc.GetDescriptorHandle();
					std::vector<Format> formats = { Format::R16G16B16A16_FLOAT };
					d3d12::CreateUAVFromRTV(render_target, rtv_out_uav_handle, 1, formats.data());
				}

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
					bool is_fallback = d3d12::GetRaytracingType(n_render_system.m_device) == RaytracingType::FALLBACK;
					d3d12::BindDescriptorHeaps(cmd_list, is_fallback);
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

		void DestroyDeferredCompositionTask(FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			if (!resize)
			{
				auto& data = fg.GetData<DeferredCompositionTaskData>(handle);

				// Small hack to force the allocations to go out of scope, which will tell the texture pool to free them
				std::move(data.out_gbuffer_albedo_alloc);
				std::move(data.out_gbuffer_normal_alloc);
				std::move(data.out_gbuffer_emissive_alloc);
				std::move(data.out_gbuffer_depth_alloc);
				std::move(data.out_lights_alloc);
				std::move(data.out_buffer_refl_shadow_alloc);
				std::move(data.out_screen_space_irradiance_alloc);
				std::move(data.out_screen_space_ao_alloc);
				std::move(data.out_output_alloc);
			}
		}

	}

	void AddDeferredCompositionTask(FrameGraph& fg, std::optional<unsigned int> target_width, std::optional<unsigned int> target_height)
	{
		std::wstring name(L"Deferred Composition");

		RenderTargetProperties rt_properties
		{
			RenderTargetProperties::IsRenderWindow(false),
			RenderTargetProperties::Width(target_width),
			RenderTargetProperties::Height(target_height),
			RenderTargetProperties::ExecuteResourceState(ResourceState::UNORDERED_ACCESS),
			RenderTargetProperties::FinishedResourceState(ResourceState::COPY_SOURCE),
			RenderTargetProperties::CreateDSVBuffer(false),
			RenderTargetProperties::DSVFormat(Format::UNKNOWN),
			RenderTargetProperties::RTVFormats({ wr::Format::R16G16B16A16_FLOAT }),
			RenderTargetProperties::NumRTVFormats(1),
			RenderTargetProperties::Clear(false),
			RenderTargetProperties::ClearDepth(false),
			RenderTargetProperties::ResourceName(name)
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::SetupDeferredCompositionTask(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle) {
			internal::ExecuteDeferredCompositionTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::DestroyDeferredCompositionTask(fg, handle, resize);
		};

		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		fg.AddTask<DeferredCompositionTaskData>(desc, fg_dep<DeferredMainTaskData, CubemapConvolutionTaskData>());
	}

}

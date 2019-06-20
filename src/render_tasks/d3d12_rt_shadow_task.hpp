#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "../d3d12/d3d12_structured_buffer_pool.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../scene_graph/scene_graph.hpp"
#include "../scene_graph/camera_node.hpp"
#include "../engine_registry.hpp"

#include "d3d12_build_acceleration_structures.hpp"
#include "d3d12_deferred_main.hpp"
#include "d3d12_rt_hybrid_helpers.hpp"

#include "../imgui_tools.hpp"

namespace wr
{
	struct RTShadowSettings
	{
		struct Runtime
		{
			float m_epsilon = 0.01f;
			int m_sample_count = 1;
		};

		Runtime m_runtime;
	};

	struct RTShadowData : RTHybrid_BaseData
	{
	};

	namespace internal
	{
		inline void SetupRTShadowTask(RenderSystem & render_system, FrameGraph & fg, RenderTaskHandle & handle, bool resize)
		{
			// Initialize variables
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);
			auto& device = n_render_system.m_device;
			auto& data = fg.GetData<RTShadowData>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);
			d3d12::SetName(n_render_target, L"Raytracing Shadow Target");

			if (!resize)
			{
				auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);

				// Get AS build data
				auto& as_build_data = fg.GetPredecessorData<wr::ASBuildData>();

				data.out_output_alloc = std::move(as_build_data.out_allocator->Allocate());
				data.out_gbuffer_albedo_alloc = std::move(as_build_data.out_allocator->Allocate());
				data.out_gbuffer_normal_alloc = std::move(as_build_data.out_allocator->Allocate());
				data.out_gbuffer_depth_alloc = std::move(as_build_data.out_allocator->Allocate());

				data.tlas_requires_init = true;

				// Camera constant buffer
				data.out_cb_camera_handle = static_cast<D3D12ConstantBufferHandle*>(n_render_system.m_raytracing_cb_pool->Create(sizeof(temp::RTHybridCamera_CBData)));

				// Pipeline State Object
				auto& rt_registry = RTPipelineRegistry::Get();
				data.out_state_object = static_cast<d3d12::StateObject*>(rt_registry.Find(as_build_data.out_using_transparency
																							? state_objects::rt_shadow_state_object_transparency
																							: state_objects::rt_shadow_state_object));

				// Root Signature
				auto& rs_registry = RootSignatureRegistry::Get();
				data.out_root_signature = static_cast<d3d12::RootSignature*>(rs_registry.Find(root_signatures::rt_hybrid_global));
			}

			// Versioning
			CreateUAVsAndSRVs(fg, data, n_render_target);

			// Create Shader Tables
			for (int i = 0; i < d3d12::settings::num_back_buffers; ++i)
			{
				CreateShaderTables(device, data, "ShadowRaygenEntry",
									{ "ShadowMissEntry" },
									{ "ShadowHitGroup" }, i);
			}

			// Setup frame index
			data.frame_idx = 0;
		}

		inline void ExecuteRTShadowTask(RenderSystem & render_system, FrameGraph & fg, SceneGraph & scene_graph, RenderTaskHandle & handle)
		{
			// Initialize variables
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
			auto& data = fg.GetData<RTShadowData>(handle);

			auto window = n_render_system.m_window.value();
			auto render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);
			auto device = n_render_system.m_device;
			auto& as_build_data = fg.GetPredecessorData<wr::ASBuildData>();
			auto frame_idx = n_render_system.GetFrameIdx();
			auto settings = fg.GetSettings<RTShadowData, RTShadowSettings>();
			float scalar = 1.0f;

			fg.WaitForPredecessorTask<CubemapConvolutionTaskData>();

			// Rebuild acceleratrion structure a 2e time for fallback
			if (d3d12::GetRaytracingType(device) == RaytracingType::FALLBACK)
			{
				d3d12::CreateOrUpdateTLAS(device, cmd_list, data.tlas_requires_init, data.out_tlas, as_build_data.out_blas_list, frame_idx);
				d3d12::UAVBarrierAS(cmd_list, as_build_data.out_tlas, frame_idx);
			}

			if (n_render_system.m_render_window.has_value())
			{
				auto& rt_registry = RTPipelineRegistry::Get();
				data.out_state_object = static_cast<d3d12::StateObject*>(rt_registry.Find(as_build_data.out_using_transparency
																							? state_objects::rt_shadow_state_object_transparency
																							: state_objects::rt_shadow_state_object));

				d3d12::BindRaytracingPipeline(cmd_list, data.out_state_object, d3d12::GetRaytracingType(device) == RaytracingType::FALLBACK);

				// Bind output, indices and materials, offsets, etc
				auto out_uav_handle = data.out_output_alloc.GetDescriptorHandle();
				d3d12::SetRTShaderUAV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_hybrid, params::RTHybridE::OUTPUT)), out_uav_handle);

				auto out_scene_ib_handle = as_build_data.out_scene_ib_alloc.GetDescriptorHandle();
				d3d12::SetRTShaderSRV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_hybrid, params::RTHybridE::INDICES)), out_scene_ib_handle);

				auto out_scene_mat_handle = as_build_data.out_scene_mat_alloc.GetDescriptorHandle();
				d3d12::SetRTShaderSRV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_hybrid, params::RTHybridE::MATERIALS)), out_scene_mat_handle);

				auto out_scene_offset_handle = as_build_data.out_scene_offset_alloc.GetDescriptorHandle();
				d3d12::SetRTShaderSRV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_hybrid, params::RTHybridE::OFFSETS)), out_scene_offset_handle);

				auto out_albedo_gbuffer_handle = data.out_gbuffer_albedo_alloc.GetDescriptorHandle();
				d3d12::SetRTShaderSRV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_hybrid, params::RTHybridE::GBUFFERS)) + 0, out_albedo_gbuffer_handle);

				auto out_normal_gbuffer_handle = data.out_gbuffer_normal_alloc.GetDescriptorHandle();
				d3d12::SetRTShaderSRV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_hybrid, params::RTHybridE::GBUFFERS)) + 1, out_normal_gbuffer_handle);

				auto out_scene_depth_handle = data.out_gbuffer_depth_alloc.GetDescriptorHandle();
				d3d12::SetRTShaderSRV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_hybrid, params::RTHybridE::GBUFFERS)) + 2, out_scene_depth_handle);

				/*
				To keep the CopyDescriptors function happy, we need to fill the descriptor table with valid descriptors
				We fill the table with a single descriptor, then overwrite some spots with the he correct textures
				If a spot is unused, then a default descriptor will be still bound, but not used in the shaders.
				Since the renderer creates a texture pool that can be used by the render tasks, and
				the texture pool also has default textures for albedo/roughness/etc... one of those textures is a good
				candidate for this.
				*/
				{
					auto texture_handle = n_render_system.GetDefaultAlbedo();
					auto* texture_resource = static_cast<wr::d3d12::TextureResource*>(texture_handle.m_pool->GetTextureResource(texture_handle));

					size_t num_textures_in_heap = COMPILATION_EVAL(rs_layout::GetSize(params::rt_hybrid, params::RTHybridE::TEXTURES));
					unsigned int heap_loc_start = COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_hybrid, params::RTHybridE::TEXTURES));

					for (size_t i = 0; i < num_textures_in_heap; ++i)
					{
						d3d12::SetRTShaderSRV(cmd_list, 0, static_cast<std::uint32_t>(heap_loc_start + i), texture_resource);
					}
				}

				// Fill descriptor heap with textures used by the scene
				for (auto material_handle : as_build_data.out_material_handles)
				{
					auto* material_internal = material_handle.m_pool->GetMaterial(material_handle);

					auto set_srv = [&data, material_internal, cmd_list](auto texture_handle)
					{
						auto* texture_internal = static_cast<wr::d3d12::TextureResource*>(texture_handle.m_pool->GetTextureResource(texture_handle));

						d3d12::SetRTShaderSRV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_hybrid, params::RTHybridE::TEXTURES)) + static_cast<std::uint32_t>(texture_handle.m_id), texture_internal);
					};

					std::array<TextureType, static_cast<size_t>(TextureType::COUNT)> types = { TextureType::ALBEDO, TextureType::NORMAL,
																							   TextureType::ROUGHNESS, TextureType::METALLIC,
																							   TextureType::EMISSIVE, TextureType::AO };

					for (auto t : types)
					{
						if (material_internal->HasTexture(t))
							set_srv(material_internal->GetTexture(t));
					}
				}

				// Get light buffer
				if (static_cast<D3D12StructuredBufferHandle*>(scene_graph.GetLightBuffer())->m_native->m_states[frame_idx] != ResourceState::NON_PIXEL_SHADER_RESOURCE)
				{
					static_cast<D3D12StructuredBufferPool*>(scene_graph.GetLightBuffer()->m_pool)->SetBufferState(scene_graph.GetLightBuffer(), ResourceState::NON_PIXEL_SHADER_RESOURCE);
				}

				DescriptorAllocation light_alloc = std::move(as_build_data.out_allocator->Allocate());
				d3d12::DescHeapCPUHandle light_handle = light_alloc.GetDescriptorHandle();
				d3d12::CreateSRVFromStructuredBuffer(static_cast<D3D12StructuredBufferHandle*>(scene_graph.GetLightBuffer())->m_native, light_handle, frame_idx);

				d3d12::DescHeapCPUHandle light_handle2 = light_alloc.GetDescriptorHandle();
				d3d12::SetRTShaderSRV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_hybrid, params::RTHybridE::LIGHTS)), light_handle2);


				// Update offset data
				n_render_system.m_raytracing_offset_sb_pool->Update(as_build_data.out_sb_offset_handle, (void*)as_build_data.out_offsets.data(), sizeof(temp::RayTracingOffset_CBData) * as_build_data.out_offsets.size(), 0);

				// Update material data
				if (as_build_data.out_materials_require_update)
				{
					n_render_system.m_raytracing_material_sb_pool->Update(as_build_data.out_sb_material_handle, (void*)as_build_data.out_materials.data(), sizeof(temp::RayTracingMaterial_CBData) * as_build_data.out_materials.size(), 0);
				}

				// Update camera constant buffer
				auto camera = scene_graph.GetActiveCamera();
				temp::RTHybridCamera_CBData cam_data;
				cam_data.m_inverse_view = DirectX::XMMatrixInverse(nullptr, camera->m_view);
				cam_data.m_inverse_projection = DirectX::XMMatrixInverse(nullptr, camera->m_projection);
				cam_data.m_inv_vp = DirectX::XMMatrixInverse(nullptr, camera->m_view * camera->m_projection);
				cam_data.m_intensity = n_render_system.temp_intensity;
				cam_data.m_frame_idx = static_cast<float>(++data.frame_idx);
				cam_data.m_epsilon = settings.m_runtime.m_epsilon;
				cam_data.m_sample_count = settings.m_runtime.m_sample_count;
				n_render_system.m_camera_pool->Update(data.out_cb_camera_handle, sizeof(temp::RTHybridCamera_CBData), 0, frame_idx, (std::uint8_t*) & cam_data); // FIXME: Uhh wrong pool?

				// Make sure the convolution pass wrote to the skybox.
				fg.WaitForPredecessorTask<CubemapConvolutionTaskData>();

				// Get skybox
				if (SkyboxNode * skybox = scene_graph.GetCurrentSkybox().get())
				{
					auto skybox_t = static_cast<d3d12::TextureResource*>(skybox->m_skybox->m_pool->GetTextureResource(skybox->m_skybox.value()));
					d3d12::SetRTShaderSRV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_hybrid, params::RTHybridE::SKYBOX)), skybox_t);

					// Get Pre-filtered environment
					auto irradiance_t = static_cast<d3d12::TextureResource*>(skybox->m_prefiltered_env_map->m_pool->GetTextureResource(skybox->m_prefiltered_env_map.value()));
					d3d12::SetRTShaderSRV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_hybrid, params::RTHybridE::PREF_ENV_MAP)), irradiance_t);

					// Get Environment Map
					irradiance_t = static_cast<d3d12::TextureResource*>(scene_graph.GetCurrentSkybox()->m_irradiance->m_pool->GetTextureResource(scene_graph.GetCurrentSkybox()->m_irradiance.value()));
					d3d12::SetRTShaderSRV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_hybrid, params::RTHybridE::IRRADIANCE_MAP)), irradiance_t);
				}

				// Get brdf lookup texture
				auto brdf_lut_text = static_cast<d3d12::TextureResource*>(n_render_system.m_brdf_lut.value().m_pool->GetTextureResource(n_render_system.m_brdf_lut.value()));
				d3d12::SetRTShaderSRV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_hybrid, params::RTHybridE::BRDF_LUT)), brdf_lut_text);

				// Transition depth to NON_PIXEL_RESOURCE
				d3d12::TransitionDepth(cmd_list, data.out_deferred_main_rt, ResourceState::DEPTH_WRITE, ResourceState::NON_PIXEL_SHADER_RESOURCE);

				d3d12::BindDescriptorHeap(cmd_list, cmd_list->m_rt_descriptor_heap.get()->GetHeap(), DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV, frame_idx, d3d12::GetRaytracingType(device) == RaytracingType::FALLBACK);
				d3d12::BindDescriptorHeaps(cmd_list, d3d12::GetRaytracingType(device) == RaytracingType::FALLBACK);
				d3d12::BindComputeConstantBuffer(cmd_list, data.out_cb_camera_handle->m_native, 2, frame_idx);

				if (d3d12::GetRaytracingType(device) == RaytracingType::NATIVE)
				{
					d3d12::BindComputeShaderResourceView(cmd_list, as_build_data.out_tlas.m_natives[frame_idx], 1);
				}
				else if (d3d12::GetRaytracingType(device) == RaytracingType::FALLBACK)
				{
					cmd_list->m_native_fallback->SetTopLevelAccelerationStructure(0, as_build_data.out_tlas.m_fallback_tlas_ptr);
				}

				/*unsigned int verts_loc = 3; rs_layout::GetHeapLoc(params::rt_hybrid, params::RTHybridE::VERTICES);
				This should be the Parameter index not the heap location, it was only working due to a ridiculous amount of luck and should be fixed, or we completely missunderstand this stuff...
				Much love, Meine and Florian*/
				d3d12::BindComputeShaderResourceView(cmd_list, as_build_data.out_scene_vb->m_buffer, 3);

				//#ifdef _DEBUG
				CreateShaderTables(device, data, "ShadowRaygenEntry",
								{ "ShadowMissEntry" },
								{ "ShadowHitGroup" }, frame_idx);
				//#endif

				scalar = fg.GetRenderTargetResolutionScale(handle);

				// Dispatch hybrid ray tracing rays
				d3d12::DispatchRays(cmd_list,
					data.out_hitgroup_shader_table[frame_idx],
					data.out_miss_shader_table[frame_idx],
					data.out_raygen_shader_table[frame_idx],
					static_cast<std::uint32_t>(std::ceil(scalar * d3d12::GetRenderTargetWidth(render_target))),
					static_cast<std::uint32_t>(std::ceil(scalar * d3d12::GetRenderTargetHeight(render_target))),
					1,
					frame_idx);

				// Transition depth back to DEPTH_WRITE
				d3d12::TransitionDepth(cmd_list, data.out_deferred_main_rt, ResourceState::NON_PIXEL_SHADER_RESOURCE, ResourceState::DEPTH_WRITE);
			}
		}

		inline void DestroyRTShadowTask(FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			auto& data = fg.GetData<RTShadowData>(handle);

			if (!resize)
			{
				// Small hack to force the allocations to go out of scope, which will tell the allocator to free them
				std::move(data.out_output_alloc);
				std::move(data.out_gbuffer_albedo_alloc);
				std::move(data.out_gbuffer_normal_alloc);
				std::move(data.out_gbuffer_depth_alloc);
			}
		}

	} /* internal */

	inline void AddRTShadowTask(FrameGraph& fg)
	{
		std::wstring name(L"Hybrid raytracing shadows");

		RenderTargetProperties rt_properties
		{
			RenderTargetProperties::IsRenderWindow(false),
			RenderTargetProperties::Width(std::nullopt),
			RenderTargetProperties::Height(std::nullopt),
			RenderTargetProperties::ExecuteResourceState(ResourceState::UNORDERED_ACCESS),
			RenderTargetProperties::FinishedResourceState(ResourceState::COPY_SOURCE),
			RenderTargetProperties::CreateDSVBuffer(false),
			RenderTargetProperties::DSVFormat(Format::UNKNOWN),
			RenderTargetProperties::RTVFormats({ Format::R16G16B16A16_FLOAT }),
			RenderTargetProperties::NumRTVFormats(1),
			RenderTargetProperties::Clear(true),
			RenderTargetProperties::ClearDepth(true),
			RenderTargetProperties::ResolutionScalar(1.0f)
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			internal::SetupRTShadowTask(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle)
		{
			internal::ExecuteRTShadowTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			internal::DestroyRTShadowTask(fg, handle, resize);
		};

		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		fg.AddTask<RTShadowData>(desc, name, FG_DEPS<DeferredMainTaskData>());
		fg.UpdateSettings<RTShadowData>(RTShadowSettings());
	}

} /* wr */
#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "../d3d12/d3d12_structured_buffer_pool.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../scene_graph/camera_node.hpp"
#include "../rt_pipeline_registry.hpp"
#include "../root_signature_registry.hpp"
#include "../engine_registry.hpp"

#include "../render_tasks/d3d12_deferred_main.hpp"
#include "../render_tasks/d3d12_build_acceleration_structures.hpp"
#include "../render_tasks/d3d12_rt_hybrid_helpers.hpp"

namespace wr
{
	struct RTAOSettings
	{
		struct Runtime
		{
			float bias = 0.05f;
			float radius = 0.5f;
			float power = 1.f; // The final AO output is pow(AO, powerExponent) // 1.0~4.0
			float max_distance = 20.f;
			int sample_count = 8;
		};//Currently setup to make work with retardedly small scenes

		Runtime m_runtime;
	};

	struct RTAOData : RTHybrid_BaseData
	{
	};
	
	namespace internal
	{
		inline void SetupAOTask(RenderSystem& render_system, FrameGraph& fg, RenderTaskHandle& handle, bool resize)
		{
			// Initialize variables
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);
			auto& device = n_render_system.m_device;
			auto& data = fg.GetData<RTAOData>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);
			d3d12::SetName(n_render_target, L"AO Target");

			if (!resize)
			{
				auto& as_build_data = fg.GetPredecessorData<wr::ASBuildData>();

				data.out_output_alloc = std::move(as_build_data.out_allocator->Allocate(1));
				data.out_gbuffer_normal_alloc = std::move(as_build_data.out_allocator->Allocate(1));
				data.out_gbuffer_depth_alloc = std::move(as_build_data.out_allocator->Allocate(1));


				// Camera constant buffer
				data.out_cb_camera_handle = static_cast<D3D12ConstantBufferHandle*>(n_render_system.m_raytracing_cb_pool->Create(sizeof(temp::RTAO_CBData)));

				// Pipeline State Object
				auto& rt_registry = RTPipelineRegistry::Get();
				data.out_state_object = static_cast<d3d12::StateObject*>(rt_registry.Find(as_build_data.out_using_transparency
																							? state_objects::rt_ao_state_object_transparency
																							: state_objects::rt_ao_state_object));

				// Root Signature
				auto& rs_registry = RootSignatureRegistry::Get();
				data.out_root_signature = static_cast<d3d12::RootSignature*>(rs_registry.Find(root_signatures::rt_ao_global));

				if (as_build_data.out_using_transparency)
				{
					// Create Shader Tables
					for (int frame_idx = 0; frame_idx < d3d12::settings::num_back_buffers; ++frame_idx)
					{
						CreateShaderTables(device, data, "AORaygenEntry_Transparency", { "AOMissEntry" }, { "AOHitGroup" }, frame_idx);
					}
				}
				else
				{
					// Create Shader Tables
					for (int frame_idx = 0; frame_idx < d3d12::settings::num_back_buffers; ++frame_idx)
					{
						CreateShaderTables(device, data, "AORaygenEntry", { "AOMissEntry" }, { }, frame_idx);
					}
				}
			}

			// Versioning
			for (int frame_idx = 0; frame_idx < 1; ++frame_idx)
			{
				// Bind output texture
				d3d12::DescHeapCPUHandle rtv_handle = data.out_output_alloc.GetDescriptorHandle();
				d3d12::CreateUAVFromSpecificRTV(n_render_target, rtv_handle, 0, n_render_target->m_create_info.m_rtv_formats[0]);

				// Bind g-buffers
				d3d12::DescHeapCPUHandle normal_gbuffer_handle = data.out_gbuffer_normal_alloc.GetDescriptorHandle(0);
				d3d12::DescHeapCPUHandle depth_buffer_handle = data.out_gbuffer_depth_alloc.GetDescriptorHandle(0);
				
				auto deferred_main_rt = data.out_deferred_main_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<DeferredMainTaskData>());

				d3d12::CreateSRVFromSpecificRTV(deferred_main_rt, normal_gbuffer_handle, 1, deferred_main_rt->m_create_info.m_rtv_formats.data()[1]);
				d3d12::CreateSRVFromDSV(deferred_main_rt, depth_buffer_handle);
			}
		}

		inline void ExecuteAOTask(RenderSystem & render_system, FrameGraph & fg, SceneGraph & scene_graph, RenderTaskHandle & handle)
		{
			// Initialize variables
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);
			auto window = n_render_system.m_window.value();
			auto render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);
			auto device = n_render_system.m_device;
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
			auto& data = fg.GetData<RTAOData>(handle);
			auto& as_build_data = fg.GetPredecessorData<wr::ASBuildData>();
			auto frame_idx = n_render_system.GetFrameIdx();
			auto settings = fg.GetSettings<RTAOData, RTAOSettings>();
			fg.WaitForPredecessorTask<CubemapConvolutionTaskData>();
			float scalar = 1.0f;

			if (n_render_system.m_render_window.has_value())
			{
				auto& rt_registry = RTPipelineRegistry::Get();
				data.out_state_object = static_cast<d3d12::StateObject*>(rt_registry.Find(as_build_data.out_using_transparency
																							? state_objects::rt_ao_state_object_transparency
																							: state_objects::rt_ao_state_object));

				d3d12::BindRaytracingPipeline(cmd_list, data.out_state_object, false);

				// Bind output, indices and materials, offsets, etc
				auto out_uav_handle = data.out_output_alloc.GetDescriptorHandle();
				d3d12::SetRTShaderUAV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_ao, params::RTAOE::OUTPUT)), out_uav_handle);

				auto out_scene_ib_handle = as_build_data.out_scene_ib_alloc.GetDescriptorHandle();
				d3d12::SetRTShaderSRV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_ao, params::RTAOE::INDICES)), out_scene_ib_handle);

				auto out_scene_mat_handle = as_build_data.out_scene_mat_alloc.GetDescriptorHandle();
				d3d12::SetRTShaderSRV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_ao, params::RTAOE::MATERIALS)), out_scene_mat_handle);

				auto out_scene_offset_handle = as_build_data.out_scene_offset_alloc.GetDescriptorHandle();
				d3d12::SetRTShaderSRV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_ao, params::RTAOE::OFFSETS)), out_scene_offset_handle);

				auto out_normal_gbuffer_handle = data.out_gbuffer_normal_alloc.GetDescriptorHandle(0);
				d3d12::SetRTShaderSRV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_ao, params::RTAOE::GBUFFERS)) + 0, out_normal_gbuffer_handle);

				auto out_scene_depth_handle = data.out_gbuffer_depth_alloc.GetDescriptorHandle();
				d3d12::SetRTShaderSRV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_ao, params::RTAOE::GBUFFERS)) + 1, out_scene_depth_handle);


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

					size_t num_textures_in_heap = COMPILATION_EVAL(rs_layout::GetSize(params::rt_ao, params::RTAOE::TEXTURES));
					unsigned int heap_loc_start = COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_ao, params::RTAOE::TEXTURES));

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

						d3d12::SetRTShaderSRV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_ao, params::RTAOE::TEXTURES)) + static_cast<std::uint32_t>(texture_handle.m_id), texture_internal);
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

				// Update offset data
				n_render_system.m_raytracing_offset_sb_pool->Update(as_build_data.out_sb_offset_handle, (void*)as_build_data.out_offsets.data(), sizeof(temp::RayTracingOffset_CBData) * as_build_data.out_offsets.size(), 0);

				// Update material data
				if (as_build_data.out_materials_require_update)
				{
					n_render_system.m_raytracing_material_sb_pool->Update(as_build_data.out_sb_material_handle, (void*)as_build_data.out_materials.data(), sizeof(temp::RayTracingMaterial_CBData) * as_build_data.out_materials.size(), 0);
				}

				// Update constant buffer
				auto camera = scene_graph.GetActiveCamera();
				temp::RTAO_CBData cb_data;
				cb_data.m_inv_vp = DirectX::XMMatrixInverse(nullptr, camera->m_view * camera->m_projection);
				cb_data.m_inv_view = DirectX::XMMatrixInverse(nullptr, camera->m_view);
				cb_data.m_bias = settings.m_runtime.bias;
				cb_data.m_radius = settings.m_runtime.radius;
				cb_data.m_power = settings.m_runtime.power;
				cb_data.m_max_distance = settings.m_runtime.max_distance;
				cb_data.m_frame_idx = frame_idx;
				cb_data.m_sample_count = static_cast<unsigned int>(settings.m_runtime.sample_count);

				n_render_system.m_camera_pool->Update(data.out_cb_camera_handle, sizeof(temp::RTAO_CBData), 0, frame_idx, (std::uint8_t*)& cb_data); // FIXME: Uhh wrong pool?

				// Transition depth to NON_PIXEL_RESOURCE
				d3d12::TransitionDepth(cmd_list, data.out_deferred_main_rt, ResourceState::DEPTH_WRITE, ResourceState::NON_PIXEL_SHADER_RESOURCE);

				d3d12::BindDescriptorHeap(cmd_list, cmd_list->m_rt_descriptor_heap.get()->GetHeap(), DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV, frame_idx, false);
				d3d12::BindDescriptorHeaps(cmd_list, false);
				d3d12::BindComputeConstantBuffer(cmd_list, data.out_cb_camera_handle->m_native, 2, frame_idx);

				if (!as_build_data.out_blas_list.empty())
				{
					d3d12::BindComputeShaderResourceView(cmd_list, as_build_data.out_tlas.m_natives[frame_idx], 1);
				}
				
				d3d12::BindComputeShaderResourceView(cmd_list, as_build_data.out_scene_vb->m_buffer, 3);

#ifdef _DEBUG
				if (as_build_data.out_using_transparency)
				{
					CreateShaderTables(device, data, "AORaygenEntry_Transparency", { "AOMissEntry" }, { "AOHitGroup" }, frame_idx);
				}
				else
				{
					CreateShaderTables(device, data, "AORaygenEntry", { "AOMissEntry" }, { }, frame_idx);
				}
#endif // _DEBUG

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

		inline void DestroyAOTask(FrameGraph& fg, RenderTaskHandle handle, bool resize) 
		{
			auto& data = fg.GetData<RTAOData>(handle);

			if (!resize)
			{
				// Small hack to force the allocations to go out of scope, which will tell the allocator to free them
				DescriptorAllocation temp1 = std::move(data.out_output_alloc);
				DescriptorAllocation temp2 = std::move(data.out_gbuffer_normal_alloc);
				DescriptorAllocation temp3 = std::move(data.out_gbuffer_depth_alloc);
			}
		}
	}
	inline void AddRTAOTask(FrameGraph& fg, d3d12::Device* device)
	{
		if (wr::d3d12::GetRaytracingType(device) == wr::RaytracingType::NATIVE)//We do not support fallback layer for shadow rays.
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
				RenderTargetProperties::RTVFormats({ Format::R8_UNORM}),
				RenderTargetProperties::NumRTVFormats(1),
				RenderTargetProperties::Clear(true),
				RenderTargetProperties::ClearDepth(true),
			};

			RenderTaskDesc desc;
			desc.m_setup_func = [](RenderSystem & rs, FrameGraph & fg, RenderTaskHandle handle, bool resize)
			{
				internal::SetupAOTask(rs, fg, handle, resize);
			};
			desc.m_execute_func = [](RenderSystem & rs, FrameGraph & fg, SceneGraph & sg, RenderTaskHandle handle)
			{
				internal::ExecuteAOTask(rs, fg, sg, handle);
			};
			desc.m_destroy_func = [](FrameGraph & fg, RenderTaskHandle handle, bool resize)
			{
				internal::DestroyAOTask(fg, handle, resize);
			};

			desc.m_properties = rt_properties;
			desc.m_type = RenderTaskType::COMPUTE;
			desc.m_allow_multithreading = true;

			fg.AddTask<RTAOData>(desc, L"Ray Traced Ambient Occlusion");
			fg.UpdateSettings<RTAOData>(RTAOSettings());
		}
		else
		{
			LOGW("RTAO task was not added since the fallback layer is not supported for RTAO. Consider using HBAO+ instead.")
		}
	}
}// namespace wr

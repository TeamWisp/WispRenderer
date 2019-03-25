#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "d3d12_deferred_main.hpp"
#include "../d3d12/d3d12_structured_buffer_pool.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../scene_graph/camera_node.hpp"
#include "../d3d12/d3d12_rt_pipeline_registry.hpp"
#include "../d3d12/d3d12_root_signature_registry.hpp"
#include "../engine_registry.hpp"
#include "../util/math.hpp"

#include "../render_tasks/d3d12_deferred_main.hpp"
#include "../render_tasks/d3d12_build_acceleration_structures.hpp"
#include "../imgui_tools.hpp"

namespace wr
{
	struct RTAOData
	{
		// Shader tables
		std::array<d3d12::ShaderTable*, d3d12::settings::num_back_buffers> in_raygen_shader_table = { nullptr, nullptr, nullptr };
		std::array<d3d12::ShaderTable*, d3d12::settings::num_back_buffers> in_miss_shader_table = { nullptr, nullptr, nullptr };
		std::array<d3d12::ShaderTable*, d3d12::settings::num_back_buffers> in_hitgroup_shader_table = { nullptr, nullptr, nullptr };

		// Pipeline objects
		d3d12::StateObject* in_state_object;
		d3d12::RootSignature* in_root_signature;

		D3D12ConstantBufferHandle* in_cb_camera_handle;
		d3d12::RenderTarget* in_deferred_main_rt;

		DescriptorAllocation out_uav_from_rtv;
		DescriptorAllocation in_gbuffers;
		DescriptorAllocation in_depthbuffer;
	};
	
	namespace internal
	{
		inline void CreateShaderTables(d3d12::Device* device, RTAOData& data, int frame_idx)
		{
			// Delete existing shader table
			if (data.in_miss_shader_table[frame_idx])
			{
				d3d12::Destroy(data.in_miss_shader_table[frame_idx]);
			}
			if (data.in_hitgroup_shader_table[frame_idx])
			{
				d3d12::Destroy(data.in_hitgroup_shader_table[frame_idx]);
			}
			if (data.in_raygen_shader_table[frame_idx])
			{
				d3d12::Destroy(data.in_raygen_shader_table[frame_idx]);
			}

			// Set up Raygen Shader Table
			{
				// Create Record(s)
				UINT shader_record_count = 1;
				auto shader_identifier_size = d3d12::GetShaderIdentifierSize(device, data.in_state_object);
				auto shader_identifier = d3d12::GetShaderIdentifier(device, data.in_state_object, "RaygenEntry");

				auto shader_record = d3d12::CreateShaderRecord(shader_identifier, shader_identifier_size);

				// Create Table
				data.in_raygen_shader_table[frame_idx] = d3d12::CreateShaderTable(device, shader_record_count, shader_identifier_size);
				d3d12::AddShaderRecord(data.in_raygen_shader_table[frame_idx], shader_record);
			}

			// Set up Miss Shader Table
			{
				// Create Record(s)
				UINT shader_record_count = 1;
				auto shader_identifier_size = d3d12::GetShaderIdentifierSize(device, data.in_state_object);

				auto miss_identifier = d3d12::GetShaderIdentifier(device, data.in_state_object, "AOMissEntry");
				auto miss_record = d3d12::CreateShaderRecord(miss_identifier, shader_identifier_size);

				// Create Table(s)
				data.in_miss_shader_table[frame_idx] = d3d12::CreateShaderTable(device, shader_record_count, shader_identifier_size);
				d3d12::AddShaderRecord(data.in_miss_shader_table[frame_idx], miss_record);
			}

			// Set up Hit Group Shader Table
			{
				// Create Record(s)
				UINT shader_record_count = 1;
				auto shader_identifier_size = d3d12::GetShaderIdentifierSize(device, data.in_state_object);

				auto hit_identifier = d3d12::GetShaderIdentifier(device, data.in_state_object, "ShadowHitGroup");
				auto hit_record = d3d12::CreateShaderRecord(hit_identifier, shader_identifier_size);

				// Create Table(s)
				data.in_hitgroup_shader_table[frame_idx] = d3d12::CreateShaderTable(device, shader_record_count, shader_identifier_size);
				d3d12::AddShaderRecord(data.in_hitgroup_shader_table[frame_idx], hit_record);
			}
		}

		inline void SetupAOTask(RenderSystem & render_system, FrameGraph & fg, RenderTaskHandle & handle, bool resize)
		{
			// Initialize variables
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);
			auto& device = n_render_system.m_device;
			auto& data = fg.GetData<RTAOData>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);
			d3d12::SetName(n_render_target, L"AO Target");

			if (!resize)
			{
				auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
				auto pred_cmd_list = fg.GetPredecessorCommandList<wr::ASBuildData>();

				cmd_list->m_rt_descriptor_heap = static_cast<d3d12::CommandList*>(pred_cmd_list)->m_rt_descriptor_heap;

				// Get AS build data
				auto& as_build_data = fg.GetPredecessorData<wr::ASBuildData>();

				data.out_uav_from_rtv = std::move(as_build_data.out_allocator->Allocate(2));
				data.in_gbuffers = std::move(as_build_data.out_allocator->Allocate(2));
				data.in_depthbuffer = std::move(as_build_data.out_allocator->Allocate());
			}

			// Bind output texture

			d3d12::DescHeapCPUHandle rtv_handle = data.out_uav_from_rtv.GetDescriptorHandle();
			d3d12::CreateUAVFromSpecificRTV(n_render_target, rtv_handle, 0, n_render_target->m_create_info.m_rtv_formats[0]);


			// Bind g-buffers (albedo, normal, depth)
			d3d12::DescHeapCPUHandle gbuffers_handle = data.in_gbuffers.GetDescriptorHandle();
			d3d12::DescHeapCPUHandle depth_buffer_handle = data.in_depthbuffer.GetDescriptorHandle();

			//cpu_handle = d3d12::GetCPUHandle(as_build_data.out_rt_heap, frame_idx, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_hybrid, params::RTHybridE::GBUFFERS)));

			auto deferred_main_rt = data.in_deferred_main_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<DeferredMainTaskData>());
			d3d12::CreateSRVFromRTV(deferred_main_rt, gbuffers_handle, 2, deferred_main_rt->m_create_info.m_rtv_formats.data());
			d3d12::CreateSRVFromDSV(deferred_main_rt, depth_buffer_handle);


			if (!resize)
			{
				// Camera constant buffer
				data.in_cb_camera_handle = static_cast<D3D12ConstantBufferHandle*>(n_render_system.m_raytracing_cb_pool->Create(sizeof(temp::RTHybridCamera_CBData)));

				// Pipeline State Object
				auto& rt_registry = RTPipelineRegistry::Get();
				data.in_state_object = static_cast<D3D12StateObject*>(rt_registry.Find(state_objects::rt_hybrid_state_object))->m_native;

				// Root Signature
				auto& rs_registry = RootSignatureRegistry::Get();
				data.in_root_signature = static_cast<D3D12RootSignature*>(rs_registry.Find(root_signatures::rt_hybrid_global))->m_native;
			}

			// Create Shader Tables
			CreateShaderTables(device, data, 0);
			CreateShaderTables(device, data, 1);
			CreateShaderTables(device, data, 2);
		}

		inline void ExecuteAOTask(RenderSystem & render_system, FrameGraph & fg, SceneGraph & scene_graph, RenderTaskHandle & handle)
		{
			// Initialize variables
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);
			auto window = n_render_system.m_window.value();
			auto device = n_render_system.m_device;
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
			auto& data = fg.GetData<RTAOData>(handle);
			auto& as_build_data = fg.GetPredecessorData<wr::ASBuildData>();

			d3d12::DescriptorHeap* desc_heap = cmd_list->m_rt_descriptor_heap->GetHeap();

			// Wait for AS to be built
			cmd_list->m_native->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(as_build_data.out_tlas.m_native));

			if (n_render_system.m_render_window.has_value())
			{
				auto frame_idx = n_render_system.GetFrameIdx();

				d3d12::BindRaytracingPipeline(cmd_list, data.in_state_object, d3d12::GetRaytracingType(device) == RaytracingType::FALLBACK);

				// Bind output, indices and materials, offsets, etc
				auto out_uav_handle = data.out_uav_from_rtv.GetDescriptorHandle();
				d3d12::SetRTShaderUAV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_ao, params::RTAOE::OUTPUT)), out_uav_handle);

				auto in_scene_gbuffers_handle1 = data.in_gbuffers.GetDescriptorHandle(0);
				d3d12::SetRTShaderSRV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_ao, params::RTAOE::GBUFFERS)) + 0, in_scene_gbuffers_handle1);

				auto in_scene_gbuffers_handle2 = data.in_gbuffers.GetDescriptorHandle(1);
				d3d12::SetRTShaderSRV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_ao, params::RTAOE::GBUFFERS)) + 1, in_scene_gbuffers_handle2);

				auto in_scene_depth_handle = data.in_depthbuffer.GetDescriptorHandle();
				d3d12::SetRTShaderSRV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_ao, params::RTAOE::GBUFFERS)) + 2, in_scene_depth_handle);


				// Fill descriptor heap with textures used by the scene
				for (auto handle : as_build_data.out_material_handles)
				{
					auto* material_internal = handle.m_pool->GetMaterial(handle.m_id);

					auto set_srv = [&data, material_internal, cmd_list](auto texture_handle)
					{
						auto* texture_internal = static_cast<wr::d3d12::TextureResource*>(texture_handle.m_pool->GetTexture(texture_handle.m_id));

						d3d12::SetRTShaderSRV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_hybrid, params::RTHybridE::TEXTURES)) + texture_handle.m_id, texture_internal);
					};

					if (!material_internal->UsesConstantAlbedo())
						set_srv(material_internal->GetAlbedo());

					set_srv(material_internal->GetNormal());

					if (!material_internal->UsesConstantMetallic())
						set_srv(material_internal->GetMetallic());

					if (!material_internal->UsesConstantRoughness())
						set_srv(material_internal->GetRoughness());
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
				cam_data.m_frame_idx = ++data.frame_idx;
				cam_data.m_shadows_enabled = camera->m_shadows_enabled;
				cam_data.m_reflections_enabled = camera->m_reflections_enabled;
				cam_data.m_ao_enabled = camera->m_ao_enabled;

				n_render_system.m_camera_pool->Update(data.out_cb_camera_handle, sizeof(temp::RTHybridCamera_CBData), 0, frame_idx, (std::uint8_t*)&cam_data); // FIXME: Uhh wrong pool?

				// Get skybox
				if (scene_graph.m_skybox.has_value())
				{
					auto skybox_t = static_cast<d3d12::TextureResource*>(scene_graph.m_skybox.value().m_pool->GetTexture(scene_graph.m_skybox.value().m_id));
					d3d12::SetRTShaderSRV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_hybrid, params::RTHybridE::SKYBOX)), skybox_t);
				}

				// Get Environment Map
				if (scene_graph.m_skybox.has_value())
				{
					auto irradiance_t = static_cast<d3d12::TextureResource*>(scene_graph.GetCurrentSkybox()->m_irradiance->m_pool->GetTexture(scene_graph.GetCurrentSkybox()->m_irradiance->m_id));
					d3d12::SetRTShaderSRV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::rt_hybrid, params::RTHybridE::IRRADIANCE_MAP)), irradiance_t);
				}

				// Transition depth to NON_PIXEL_RESOURCE
				d3d12::TransitionDepth(cmd_list, data.out_deferred_main_rt, ResourceState::DEPTH_WRITE, ResourceState::NON_PIXEL_SHADER_RESOURCE);

				d3d12::BindDescriptorHeap(cmd_list, cmd_list->m_rt_descriptor_heap.get()->GetHeap(), DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV, frame_idx, d3d12::GetRaytracingType(device) == RaytracingType::FALLBACK);
				d3d12::BindDescriptorHeaps(cmd_list, frame_idx, d3d12::GetRaytracingType(device) == RaytracingType::FALLBACK);
				d3d12::BindComputeConstantBuffer(cmd_list, data.out_cb_camera_handle->m_native, 2, frame_idx);

				if (d3d12::GetRaytracingType(device) == RaytracingType::NATIVE)
				{
					d3d12::BindComputeShaderResourceView(cmd_list, as_build_data.out_tlas.m_native, 1);
				}
				else if (d3d12::GetRaytracingType(device) == RaytracingType::FALLBACK)
				{
					cmd_list->m_native_fallback->SetTopLevelAccelerationStructure(0, as_build_data.out_tlas.m_fallback_tlas_ptr);
				}

				unsigned int verts_loc = 3;/* rs_layout::GetHeapLoc(params::rt_hybrid, params::RTHybridE::VERTICES);
				This should be the Parameter index not the heap location, it was only working due to a ridiculous amount of luck and should be fixed, or we completely missunderstand this stuff...
				Much love, Meine and Florian*/
				d3d12::BindComputeShaderResourceView(cmd_list, as_build_data.out_scene_vb->m_buffer, verts_loc);

#ifdef _DEBUG
				CreateShaderTables(device, data, frame_idx);
#endif // _DEBUG

				// Dispatch hybrid ray tracing rays
				d3d12::DispatchRays(cmd_list, data.out_hitgroup_shader_table[frame_idx], data.out_miss_shader_table[frame_idx], data.out_raygen_shader_table[frame_idx], window->GetWidth(), window->GetHeight(), 1, frame_idx);

				// Transition depth back to DEPTH_WRITE
				d3d12::TransitionDepth(cmd_list, data.out_deferred_main_rt, ResourceState::NON_PIXEL_SHADER_RESOURCE, ResourceState::DEPTH_WRITE);
			}
		}

		inline void AddAOTask(FrameGraph& fg)
		{
			std::wstring name(L"Ambient Oclussion");
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
				RenderTargetProperties::ResourceName(name)
			};

			RenderTaskDesc desc;
			desc.m_setup_func = [](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
			{
				internal::SetupAOTask(rs, fg, handle, resize);
			};
			desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle)
			{
				internal::ExecuteAOTask(rs, fg, sg, handle);
			};
			desc.m_destroy_func = [](FrameGraph& fg, RenderTaskHandle handle, bool resize)
			{
				internal::DestroyAOTask(fg, handle, resize);
			};

			desc.m_properties = rt_properties;
			desc.m_type = RenderTaskType::COMPUTE;
			desc.m_allow_multithreading = true;

			fg.AddTask<RTAOData>(desc);
		}
	}
}// namespace wr
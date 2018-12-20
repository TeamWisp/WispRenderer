#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "../d3d12/d3d12_structured_buffer_pool.hpp"
#include "../frame_graph/render_task.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../scene_graph/camera_node.hpp"
#include "../d3d12/d3d12_rt_pipeline_registry.hpp"
#include "../d3d12/d3d12_root_signature_registry.hpp"
#include "../engine_registry.hpp"
#include "../util/math.hpp"

#include "../render_tasks/d3d12_deferred_main.hpp"
#include "../imgui_tools.hpp"


namespace wr
{
	struct RaytracingData
	{
		d3d12::DescriptorHeap* out_rt_heap;

		std::array<d3d12::ShaderTable*, d3d12::settings::num_back_buffers> out_raygen_shader_table = { nullptr, nullptr, nullptr };
		std::array<d3d12::ShaderTable*, d3d12::settings::num_back_buffers> out_miss_shader_table = { nullptr, nullptr, nullptr };
		std::array<d3d12::ShaderTable*, d3d12::settings::num_back_buffers> out_hitgroup_shader_table = { nullptr, nullptr, nullptr };
		d3d12::StateObject* out_state_object;
		d3d12::RootSignature* out_root_signature;
		D3D12ConstantBufferHandle* out_cb_camera_handle;
		D3D12StructuredBufferHandle* out_sb_material_handle;

		bool out_init;
	};
	using RaytracingTask = RenderTask<RaytracingData>;

	namespace internal
	{

		inline void CreateShaderTables(d3d12::Device* device, RaytracingData& data, int frame_idx)
		{
			if (data.out_miss_shader_table[frame_idx])
			{
				d3d12::Destroy(data.out_miss_shader_table[frame_idx]);
			}
			if (data.out_hitgroup_shader_table[frame_idx])
			{
				d3d12::Destroy(data.out_hitgroup_shader_table[frame_idx]);
			}
			if (data.out_raygen_shader_table[frame_idx])
			{
				d3d12::Destroy(data.out_raygen_shader_table[frame_idx]);
			}

			// Raygen Shader Table
			{
				// Create Record(s)
				UINT shader_record_count = 1;
				auto shader_identifier_size = d3d12::GetShaderIdentifierSize(device, data.out_state_object);
				auto shader_identifier = d3d12::GetShaderIdentifier(device, data.out_state_object, "RaygenEntry");

				auto shader_record = d3d12::CreateShaderRecord(shader_identifier, shader_identifier_size);

				// Create Table
				data.out_raygen_shader_table[frame_idx] = d3d12::CreateShaderTable(device, shader_record_count, shader_identifier_size);
				d3d12::AddShaderRecord(data.out_raygen_shader_table[frame_idx], shader_record);
			}

			// Miss Shader Table
			{
				// Create Record(s)
				UINT shader_record_count = 1;
				auto shader_identifier_size = d3d12::GetShaderIdentifierSize(device, data.out_state_object);
				auto shader_identifier = d3d12::GetShaderIdentifier(device, data.out_state_object, "MissEntry");

				auto shader_record = d3d12::CreateShaderRecord(shader_identifier, shader_identifier_size);

				// Create Table
				data.out_miss_shader_table[frame_idx] = d3d12::CreateShaderTable(device, shader_record_count, shader_identifier_size);
				d3d12::AddShaderRecord(data.out_miss_shader_table[frame_idx], shader_record);
			}

			// Hit Group Shader Table
			{
				// Create Record(s)
				UINT shader_record_count = 1;
				auto shader_identifier_size = d3d12::GetShaderIdentifierSize(device, data.out_state_object);
				auto shader_identifier = d3d12::GetShaderIdentifier(device, data.out_state_object, "MyHitGroup");

				auto shader_record = d3d12::CreateShaderRecord(shader_identifier, shader_identifier_size);

				// Create Table
				data.out_hitgroup_shader_table[frame_idx] = d3d12::CreateShaderTable(device, shader_record_count, shader_identifier_size);
				d3d12::AddShaderRecord(data.out_hitgroup_shader_table[frame_idx], shader_record);
			}
		}

		inline void SetupRaytracingTask(RenderSystem & render_system, RaytracingTask & task, RaytracingData & data)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);
			auto& device = n_render_system.m_device;
			auto n_render_target = static_cast<d3d12::RenderTarget*>(task.GetRenderTarget<RenderTarget>());

			n_render_target->m_render_targets[0]->SetName(L"Raytracing Target");

			data.out_init = true;

			// top level, bottom level and output buffer. (even though I don't use bottom level.)
			d3d12::desc::DescriptorHeapDesc heap_desc;
			heap_desc.m_num_descriptors = 100; // FIXME: size
			heap_desc.m_type = DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV;
			heap_desc.m_shader_visible = true;
			heap_desc.m_versions = d3d12::settings::num_back_buffers;
			data.out_rt_heap = d3d12::CreateDescriptorHeap(device, heap_desc);
			SetName(data.out_rt_heap, L"Raytracing Task Descriptor Heap");

			// materials are not versioned atm

			data.out_sb_material_handle = static_cast<D3D12StructuredBufferHandle*>(n_render_system.m_raytracing_material_sb_pool->Create(sizeof(temp::RayTracingMaterial_CBData) * d3d12::settings::num_max_rt_materials, sizeof(temp::RayTracingMaterial_CBData), false));
			// Pipeline State Object
			auto rt_registry = RTPipelineRegistry::Get();
			data.out_state_object = static_cast<D3D12StateObject*>(rt_registry.Find(state_objects::state_object))->m_native;

			// Root Signature
			auto rs_registry = RootSignatureRegistry::Get();
			data.out_root_signature = static_cast<D3D12RootSignature*>(rs_registry.Find(root_signatures::rt_test_global))->m_native;

			// Camera constant buffer
			data.out_cb_camera_handle = static_cast<D3D12ConstantBufferHandle*>(n_render_system.m_raytracing_cb_pool->Create(sizeof(temp::RayTracingCamera_CBData)));


			for (auto frame_idx = 0; frame_idx < 1; frame_idx++)
			{
				auto cpu_handle = d3d12::GetCPUHandle(data.out_rt_heap, frame_idx);
				d3d12::CreateSRVFromSpecificUAV(n_render_target, cpu_handle, frame_idx, n_render_target->m_create_info.m_rtv_formats[frame_idx]);

			}
			
			CreateShaderTables(device, data, 0);
			CreateShaderTables(device, data, 1);
			CreateShaderTables(device, data, 2);
		}

		d3d12::AccelerationStructure tlas;
		std::vector<std::pair<std::pair<d3d12::AccelerationStructure, unsigned int>, DirectX::XMMATRIX>> blas_list;

		std::vector<std::shared_ptr<D3D12ModelPool>> model_pools;

		d3d12::StagingBuffer* temp_ib;
		d3d12::StagingBuffer* temp_vb;

		std::vector<temp::RayTracingMaterial_CBData> materials(d3d12::settings::num_max_rt_materials);
		int fc = -10;
		inline void ExecuteRaytracingTask(RenderSystem & render_system, RaytracingTask & task, SceneGraph & scene_graph, RaytracingData & data)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);
			auto window = n_render_system.m_window.value();
			auto device = n_render_system.m_device;
			auto cmd_list = task.GetCommandList<d3d12::CommandList>().first;

			if (n_render_system.m_render_window.has_value())
			{
				// Initialize requirements
				if (data.out_init)
				{
					model_pools = n_render_system.m_model_pools;
					// Transition all model pools for accel structure creation
					for (auto& pool : model_pools)
					{
						d3d12::Transition(cmd_list, pool->GetVertexStagingBuffer(), ResourceState::VERTEX_AND_CONSTANT_BUFFER, ResourceState::NON_PIXEL_SHADER_RESOURCE);
						d3d12::Transition(cmd_list, pool->GetIndexStagingBuffer(), ResourceState::INDEX_BUFFER, ResourceState::NON_PIXEL_SHADER_RESOURCE);
					}

					unsigned int material_id = 0;

					// List all materials used by meshes
					std::vector<MaterialHandle*> material_handles;

					// Create Geometry from scene graph
					{
						scene_graph.Optimize();
						auto batches = scene_graph.GetBatches();

						for (auto& batch : batches)
						{
							auto n_model_pool = static_cast<D3D12ModelPool*>(batch.first->m_model_pool);
							auto vb = n_model_pool->GetVertexStagingBuffer();
							auto ib = n_model_pool->GetIndexStagingBuffer();
							auto model = batch.first;

							temp_ib = ib;
							temp_vb = vb;

							for (auto& mesh : model->m_meshes)
							{
								auto n_mesh = static_cast<D3D12ModelPool*>(model->m_model_pool)->GetMeshData(mesh.first->id);

								d3d12::desc::GeometryDesc obj;
								obj.index_buffer = ib;
								obj.vertex_buffer = vb;

								obj.m_indices_offset = n_mesh->m_index_staging_buffer_offset;
								obj.m_num_indices = n_mesh->m_index_count;
								obj.m_vertices_offset = n_mesh->m_vertex_staging_buffer_offset;
								obj.m_num_vertices = n_mesh->m_vertex_count;
								obj.m_vertex_stride = n_mesh->m_vertex_staging_buffer_stride;

								// Build Bottom level BVH
								auto blas = d3d12::CreateBottomLevelAccelerationStructures(device, cmd_list, data.out_rt_heap, { obj });
								cmd_list->m_native->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(blas.m_native));
								blas.m_native->SetName(L"Bottomlevelaccel");

								material_handles.push_back(mesh.second); // Used to steal the textures from the texture pool.

								// Push instances into a array for later use.
								for (auto i = 0; i < batch.second.num_instances; i++)
								{
									auto transform = batch.second.data.objects[i].m_model;

									// Build material
									auto* material_internal = mesh.second->m_pool->GetMaterial(mesh.second->m_id);
									materials[material_id].idx_offset = n_mesh->m_index_staging_buffer_offset;
									materials[material_id].vertex_offset = n_mesh->m_vertex_staging_buffer_offset;
									materials[material_id].albedo_id = material_internal->GetAlbedo().m_id;
									materials[material_id].normal_id = material_internal->GetNormal().m_id;
									materials[material_id].roughness_id = material_internal->GetRoughness().m_id;
									materials[material_id].metallicness_id = material_internal->GetMetallic().m_id;

									auto upper3x3 = util::StripTranslationAndLowerVector(transform);

									materials[material_id].m_model = XMMatrixTranspose(XMMatrixInverse(nullptr, upper3x3));;

									blas_list.push_back({ std::make_pair(blas, material_id), transform });

									material_id++;
								}

							}
						}
					}

					tlas = d3d12::CreateTopLevelAccelerationStructure(device, cmd_list, data.out_rt_heap, blas_list);
					tlas.m_native->SetName(L"Highlevelaccel");

					//tlas.m_native->
					CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
						tlas.m_native,
						D3D12_RESOURCE_STATE_GENERIC_READ,
						D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
					);
					//cmd_list->m_native->ResourceBarrier(1, &barrier);

					// Transition all model pools back to whatever they were.
					for (auto& pool : model_pools)
					{
						d3d12::Transition(cmd_list, pool->GetVertexStagingBuffer(), ResourceState::NON_PIXEL_SHADER_RESOURCE, ResourceState::VERTEX_AND_CONSTANT_BUFFER);
						d3d12::Transition(cmd_list, pool->GetIndexStagingBuffer(), ResourceState::NON_PIXEL_SHADER_RESOURCE, ResourceState::INDEX_BUFFER);
					}

					for (auto i = 0; i < d3d12::settings::num_back_buffers; i++)
					{
						// Create BYTE ADDRESS buffer view into a staging buffer. Hopefully this works.
						auto& cpu_handle = d3d12::GetCPUHandle(data.out_rt_heap, i);
						d3d12::Offset(cpu_handle, 1, data.out_rt_heap->m_increment_size); // Skip UAV at positon 0
						d3d12::CreateRawSRVFromStagingBuffer(temp_ib, cpu_handle, 0, temp_ib->m_size / temp_ib->m_stride_in_bytes);

						d3d12::Offset(cpu_handle, 1, data.out_rt_heap->m_increment_size); // Skip position 2 since that is for the light buffer.

						// Create material structured buffer view
						d3d12::CreateSRVFromStructuredBuffer(data.out_sb_material_handle->m_native, cpu_handle, 0);

						// Fill descriptor heap with textures used by the scene
						for (auto handle : material_handles)
						{
							auto* material_internal = handle->m_pool->GetMaterial(handle->m_id);

							auto create_srv = [data, material_internal, i](auto texture_handle)
							{
								auto cpu_handle = d3d12::GetCPUHandle(data.out_rt_heap, i);
								auto* texture_internal = static_cast<wr::d3d12::TextureResource*>(texture_handle.m_pool->GetTexture(texture_handle.m_id));

								d3d12::Offset(cpu_handle, 4 + texture_handle.m_id, data.out_rt_heap->m_increment_size);
								d3d12::CreateSRVFromTexture(texture_internal, cpu_handle);
							};

							create_srv(material_internal->GetAlbedo());
							create_srv(material_internal->GetMetallic());
							create_srv(material_internal->GetNormal());
							create_srv(material_internal->GetRoughness());
						}
					}

					data.out_init = false;
				}

				auto frame_idx = n_render_system.GetFrameIdx();

				auto n_render_target = static_cast<d3d12::RenderTarget*>(task.GetRenderTarget<RenderTarget>());
				auto cpu_handle2 = d3d12::GetCPUHandle(data.out_rt_heap, 0);
				d3d12::CreateSRVFromSpecificUAV(n_render_target, cpu_handle2, 0, n_render_target->m_create_info.m_rtv_formats[0]);

				// Get light buffer
				if (static_cast<D3D12StructuredBufferHandle*>(scene_graph.GetLightBuffer())->m_native->m_states[frame_idx] != ResourceState::NON_PIXEL_SHADER_RESOURCE)
				{
					static_cast<D3D12StructuredBufferPool*>(scene_graph.GetLightBuffer()->m_pool)->SetBufferState(scene_graph.GetLightBuffer(), ResourceState::NON_PIXEL_SHADER_RESOURCE);
				}
				auto cpu_handle = d3d12::GetCPUHandle(data.out_rt_heap, 0, 2); // here
				d3d12::CreateSRVFromStructuredBuffer(static_cast<D3D12StructuredBufferHandle*>(scene_graph.GetLightBuffer())->m_native, cpu_handle, frame_idx);

				// Update material data
				n_render_system.m_raytracing_material_sb_pool->Update(data.out_sb_material_handle, materials.data(), sizeof(temp::RayTracingMaterial_CBData) * d3d12::settings::num_max_rt_materials, 0);

				// Update camera cb
				if (n_render_system.clear_path)
				{
					fc = -1;
					n_render_system.clear_path = false;
				}


				n_render_system.temp_rough = fc;
				fc++;

				auto camera = scene_graph.GetActiveCamera();
				temp::RayTracingCamera_CBData cam_data;
				cam_data.m_view = camera->m_view;
				cam_data.m_camera_position = camera->m_position;
				cam_data.m_inverse_view_projection = DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(nullptr, camera->m_view * camera->m_projection));
				cam_data.roughness = fc;
				cam_data.metal = n_render_system.temp_metal;
				cam_data.light_radius = n_render_system.light_radius;
				cam_data.intensity = n_render_system.temp_intensity;
				n_render_system.m_camera_pool->Update(data.out_cb_camera_handle, sizeof(temp::RayTracingCamera_CBData), 0, frame_idx, (std::uint8_t*)&cam_data); // FIXME: Uhh wrong pool?

				d3d12::BindRaytracingPipeline(cmd_list, data.out_state_object, d3d12::GetRaytracingType(device) == RaytracingType::FALLBACK);

				d3d12::BindDescriptorHeaps(cmd_list, { data.out_rt_heap }, 0, d3d12::GetRaytracingType(device) == RaytracingType::FALLBACK); // here

				auto gpu_handle = d3d12::GetGPUHandle(data.out_rt_heap, 0); // here
				d3d12::BindComputeDescriptorTable(cmd_list, gpu_handle, 0);
				d3d12::BindComputeConstantBuffer(cmd_list, data.out_cb_camera_handle->m_native, 2, frame_idx);

				if (d3d12::GetRaytracingType(device) == RaytracingType::NATIVE)
				{
					d3d12::BindComputeShaderResourceView(cmd_list, tlas.m_native, 1);
				}
				else if(d3d12::GetRaytracingType(device) == RaytracingType::FALLBACK)
				{
					cmd_list->m_native_fallback->SetTopLevelAccelerationStructure(0, tlas.m_fallback_tlas_ptr);
				}

				d3d12::BindComputeShaderResourceView(cmd_list, temp_vb->m_buffer, 3);

#ifdef _DEBUG
				//CreateShaderTables(device, data, frame_idx);
#endif

				d3d12::DispatchRays(cmd_list, data.out_hitgroup_shader_table[frame_idx], data.out_miss_shader_table[frame_idx], data.out_raygen_shader_table[frame_idx], window->GetWidth(), window->GetHeight(), 1);
			}
		}

		inline void DestroyRaytracingTask(RaytracingTask & task, RaytracingData& data)
		{
		}

	} /* internal */


	//! Used to create a new defferred task.
	[[nodiscard]] inline std::unique_ptr<RaytracingTask> GetRaytracingTask()
	{
		auto ptr = std::make_unique<RaytracingTask>(nullptr, "Deferred Render Task", RenderTaskType::COMPUTE, true,
			RenderTargetProperties{
				false,
				std::nullopt,
				std::nullopt,
				ResourceState::UNORDERED_ACCESS,
				ResourceState::COPY_SOURCE,
				false,
				Format::UNKNOWN,
				{ Format::R32G32B32A32_FLOAT },
				1,
				true,
				true
			},
			[](RenderSystem & render_system, RaytracingTask & task, RaytracingData & data, bool) { internal::SetupRaytracingTask(render_system, task, data); },
			[](RenderSystem & render_system, RaytracingTask & task, SceneGraph & scene_graph, RaytracingData & data) { internal::ExecuteRaytracingTask(render_system, task, scene_graph, data); },
			[](RaytracingTask & task, RaytracingData & data, bool) { internal::DestroyRaytracingTask(task, data); }
		);

		return ptr;
	}

} /* wr */

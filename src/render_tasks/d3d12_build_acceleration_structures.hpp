#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "../d3d12/d3d12_structured_buffer_pool.hpp"
#include "../d3d12/d3d12_rt_descriptor_heap.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../engine_registry.hpp"
#include "../util/math.hpp"

#include "../render_tasks/d3d12_deferred_main.hpp"
#include "../imgui_tools.hpp"

namespace wr
{
	struct ASBuildData
	{
		DescriptorAllocator* out_allocator;
		DescriptorAllocation out_scene_ib_alloc;
		DescriptorAllocation out_scene_mat_alloc;
		DescriptorAllocation out_scene_offset_alloc;

		d3d12::AccelerationStructure out_tlas;
		D3D12StructuredBufferHandle* out_sb_material_handle;
		D3D12StructuredBufferHandle* out_sb_offset_handle;
		std::vector<std::tuple<d3d12::AccelerationStructure, unsigned int, DirectX::XMMATRIX>> out_blas_list;
		std::vector<temp::RayTracingMaterial_CBData> out_materials;
		std::vector<temp::RayTracingOffset_CBData> out_offsets;
		std::unordered_map<unsigned int, unsigned int> out_parsed_materials;
		std::vector<MaterialHandle> out_material_handles;
		d3d12::StagingBuffer* out_scene_ib;
		d3d12::StagingBuffer* out_scene_vb;

		std::unordered_map<std::uint64_t, d3d12::AccelerationStructure> blasses;

		std::vector<std::vector<d3d12::AccelerationStructure>> old_blasses;
		std::vector<d3d12::AccelerationStructure> old_tlas;

		unsigned int previous_frame_index;
		unsigned int current_frame_index;

		bool out_init;
		bool out_materials_require_update;
	};

	namespace internal
	{

		inline void SetupBuildASTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			if (resize) return;

			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& device = n_render_system.m_device;
			auto& data = fg.GetData<ASBuildData>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);
			d3d12::SetName(n_render_target, L"Raytracing Target");

			data.out_init = true;
			data.out_materials_require_update = true;

			// Structured buffer for the materials.
			data.out_sb_material_handle = static_cast<D3D12StructuredBufferHandle*>(n_render_system.m_raytracing_material_sb_pool->Create(sizeof(temp::RayTracingMaterial_CBData) * d3d12::settings::num_max_rt_materials, sizeof(temp::RayTracingMaterial_CBData), false));

			// Structured buffer for the materials.
			data.out_sb_offset_handle = static_cast<D3D12StructuredBufferHandle*>(n_render_system.m_raytracing_offset_sb_pool->Create(sizeof(temp::RayTracingOffset_CBData) * d3d12::settings::num_max_rt_materials, sizeof(temp::RayTracingOffset_CBData), false));

			// Resize the materials
			data.out_materials.reserve(d3d12::settings::num_max_rt_materials);
			data.out_offsets.reserve(d3d12::settings::num_max_rt_materials);
			data.out_parsed_materials.reserve(d3d12::settings::num_max_rt_materials);

			
			auto texture_pool = std::static_pointer_cast<D3D12TexturePool>(n_render_system.GetDefaultTexturePool());

			data.out_allocator = texture_pool->GetAllocator(DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV);

			data.out_scene_ib_alloc = std::move(data.out_allocator->Allocate());
			data.out_scene_mat_alloc = std::move(data.out_allocator->Allocate());
			data.out_scene_offset_alloc = std::move(data.out_allocator->Allocate());

			data.old_blasses.resize(d3d12::settings::num_back_buffers);
			data.old_tlas.resize(d3d12::settings::num_back_buffers);
		}

		namespace internal
		{

			//! Get a material id from a mesh.
			inline unsigned int ExtractMaterialFromMesh(ASBuildData& data, MaterialHandle material_handle)
			{
				unsigned int material_id = 0;
				if (data.out_parsed_materials.find(material_handle.m_id) == data.out_parsed_materials.end())
				{
					material_id = data.out_materials.size();

					auto* material_internal = material_handle.m_pool->GetMaterial(material_handle.m_id);

					// Build material
					wr::temp::RayTracingMaterial_CBData material;
					material.albedo_id = material_internal->GetAlbedo().m_id;
					material.normal_id = material_internal->GetNormal().m_id;
					material.roughness_id = material_internal->GetRoughness().m_id;
					material.metallicness_id = material_internal->GetMetallic().m_id;
					material.material_data = material_internal->GetMaterialData();
					int x = sizeof(wr::temp::RayTracingMaterial_CBData);
					int y = sizeof(DirectX::XMVECTOR);
					int z = sizeof(Material::MaterialData);
					data.out_materials.push_back(material);
					data.out_parsed_materials[material_handle.m_id] = material_id;

					data.out_materials_require_update = true;
				}
				else
				{
					material_id = data.out_parsed_materials[material_handle.m_id];
				}

				return material_id;
			}

			//! Add a data struct describing the mesh data offset and the material idx to `out_offsets`
			inline void AppendOffset(ASBuildData& data, wr::internal::D3D12MeshInternal* mesh, unsigned int material_id)
			{
				wr::temp::RayTracingOffset_CBData offset;
				offset.material_idx = material_id;
				offset.idx_offset = mesh->m_index_staging_buffer_offset;
				offset.vertex_offset = mesh->m_vertex_staging_buffer_offset;
				data.out_offsets.push_back(offset);
			}

			inline void BuildBLASSingle(d3d12::Device* device, d3d12::CommandList* cmd_list, Model* model, std::pair<Mesh*, MaterialHandle> mesh_material, ASBuildData& data)
			{
				auto n_model_pool = static_cast<D3D12ModelPool*>(model->m_model_pool);
				auto vb = n_model_pool->GetVertexStagingBuffer();
				auto ib = n_model_pool->GetIndexStagingBuffer();

				d3d12::DescriptorHeap* out_heap = cmd_list->m_rt_descriptor_heap->GetHeap();
				
				data.out_scene_ib = ib;
				data.out_scene_vb = vb;

				auto n_mesh = static_cast<D3D12ModelPool*>(model->m_model_pool)->GetMeshData(mesh_material.first->id);

				d3d12::desc::GeometryDesc obj;
				obj.index_buffer = ib;
				obj.vertex_buffer = vb;

				obj.m_indices_offset = n_mesh->m_index_staging_buffer_offset;
				obj.m_num_indices = n_mesh->m_index_count;
				obj.m_vertices_offset = n_mesh->m_vertex_staging_buffer_offset;
				obj.m_num_vertices = n_mesh->m_vertex_count;
				obj.m_vertex_stride = n_mesh->m_vertex_staging_buffer_stride;

				// Build Bottom level BVH
				auto blas = d3d12::CreateBottomLevelAccelerationStructures(device, cmd_list, out_heap, { obj });
				cmd_list->m_native->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(blas.m_native));
				blas.m_native->SetName(L"Bottomlevelaccel");

				data.blasses.insert({ mesh_material.first->id, blas });
				
				data.out_material_handles.push_back(mesh_material.second); // Used to st eal the textures from the texture pool.

				auto material_id = ExtractMaterialFromMesh(data, mesh_material.second);

				AppendOffset(data, n_mesh, material_id);
			}

			inline void BuildBLASList(d3d12::Device* device, d3d12::CommandList* cmd_list, SceneGraph& scene_graph, ASBuildData& data)
			{
				data.out_materials.clear();
				data.out_material_handles.clear();
				data.out_offsets.clear();
				data.out_parsed_materials.clear();

				d3d12::DescriptorHeap* out_heap = cmd_list->m_rt_descriptor_heap->GetHeap();

				std::unordered_map<uint64_t, wr::d3d12::AccelerationStructure>::iterator it;
				for (it = data.blasses.begin(); it != data.blasses.end(); ++it)
				{
					data.old_blasses[data.current_frame_index].push_back((*it).second);
				}

				data.blasses.clear();
				data.out_blas_list.clear();

				auto& batches = scene_graph.GetGlobalBatches();
				const auto& batchInfo = scene_graph.GetBatches();

				unsigned int offset_id = 0;

				for (auto& batch : batches)
				{
					auto model = batch.first.first;
					auto materials = batch.first.second;
					auto n_model_pool = static_cast<D3D12ModelPool*>(model->m_model_pool);
					auto vb = n_model_pool->GetVertexStagingBuffer();
					auto ib = n_model_pool->GetIndexStagingBuffer();

					data.out_scene_ib = ib;
					data.out_scene_vb = vb;

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
						auto blas = d3d12::CreateBottomLevelAccelerationStructures(device, cmd_list, out_heap, { obj });
						cmd_list->m_native->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(blas.m_native));
						blas.m_native->SetName(L"Bottomlevelaccel");

						data.blasses.insert({ mesh.first->id, blas });

						data.out_material_handles.push_back(mesh.second); // Used to st eal the textures from the texture pool.

						auto material_id = ExtractMaterialFromMesh(data, mesh.second);

						AppendOffset(data, n_mesh, material_id);

						auto it = batchInfo.find({ model, materials });

						assert(it != batchInfo.end(), "Batch was found in global array, but not in local");

						// Push instances into a array for later use.
						for (uint32_t i = 0U, j = (uint32_t)it->second.num_global_instances; i < j; i++)
						{
							auto transform = batch.second[i].m_model;

							data.out_blas_list.push_back({ blas, offset_id, transform });
						}

						offset_id++;
					}
				}

				// Make sure our gathered data isn't out of bounds.
				if (data.out_offsets.size() > d3d12::settings::num_max_rt_materials)
				{
					LOGE("There are to many offsets stored for ray tracing!");
				}
				if (data.out_materials.size() > d3d12::settings::num_max_rt_materials)
				{
					LOGE("There are to many materials stored for ray tracing!");
				}
			}

			inline void CreateSRVs(d3d12::CommandList* cmd_list, ASBuildData& data)
			{
				for (auto i = 0; i < d3d12::settings::num_back_buffers; i++)
				{
					// Create BYTE ADDRESS buffer view into a staging buffer. Hopefully this works.
					{
						auto cpu_handle = data.out_scene_ib_alloc.GetDescriptorHandle();
						d3d12::CreateRawSRVFromStagingBuffer(data.out_scene_ib, cpu_handle, 0, data.out_scene_ib->m_size / data.out_scene_ib->m_stride_in_bytes);
					}

					// Create material structured buffer view
					{
						auto cpu_handle = data.out_scene_mat_alloc.GetDescriptorHandle();
						d3d12::CreateSRVFromStructuredBuffer(data.out_sb_material_handle->m_native, cpu_handle, 0);
					}

					// Create offset structured buffer view
					{
						auto cpu_handle = data.out_scene_offset_alloc.GetDescriptorHandle();
						d3d12::CreateSRVFromStructuredBuffer(data.out_sb_offset_handle->m_native, cpu_handle, 0);
					}
				}
			}

			inline void UpdateTLAS(d3d12::Device* device, d3d12::CommandList* cmd_list, SceneGraph& scene_graph, ASBuildData& data)
			{
				data.out_materials.clear();
				data.out_offsets.clear();
				data.out_parsed_materials.clear();

				d3d12::DescriptorHeap* out_heap = cmd_list->m_rt_descriptor_heap->GetHeap();

				auto& batches = scene_graph.GetGlobalBatches();
				const auto& batchInfo = scene_graph.GetBatches();

				bool needs_reconstruction = false;

				std::vector<D3D12ModelPool*> model_pools;

				for (auto& batch : batches)
				{
					auto model = batch.first.first;
					auto materials = batch.first.second;

					bool model_pool_loaded = false;

					for (int i = 0; i < model_pools.size(); ++i)
					{
						if (model_pools[i] == model->m_model_pool)
						{
							model_pool_loaded = true;
						}
					}

					if (!model_pool_loaded)
					{
						model_pools.push_back(static_cast<D3D12ModelPool*>(model->m_model_pool));
						if (static_cast<D3D12ModelPool*>(model->m_model_pool)->IsUpdated())
						{
							needs_reconstruction = true;
						}
					}
				}

				if (needs_reconstruction)
				{
					// Transition all model pools for accel structure creation
					for (auto& pool : model_pools)
					{
						d3d12::Transition(cmd_list, pool->GetVertexStagingBuffer(), ResourceState::VERTEX_AND_CONSTANT_BUFFER, ResourceState::NON_PIXEL_SHADER_RESOURCE);
						d3d12::Transition(cmd_list, pool->GetIndexStagingBuffer(), ResourceState::INDEX_BUFFER, ResourceState::NON_PIXEL_SHADER_RESOURCE);
					}

					BuildBLASList(device, cmd_list, scene_graph, data);

					for (auto& pool : model_pools)
					{
						d3d12::Transition(cmd_list, pool->GetVertexStagingBuffer(), ResourceState::NON_PIXEL_SHADER_RESOURCE, ResourceState::VERTEX_AND_CONSTANT_BUFFER);
						d3d12::Transition(cmd_list, pool->GetIndexStagingBuffer(), ResourceState::NON_PIXEL_SHADER_RESOURCE, ResourceState::INDEX_BUFFER);
					}

					CreateSRVs(cmd_list, data);
				}



				auto prev_size = data.out_blas_list.size();
				data.out_blas_list.clear();
				data.out_blas_list.reserve(prev_size);

				unsigned int offset_id = 0;

				// Update transformations
				for (auto& batch : batches)
				{
					auto model = batch.first.first;
					auto materials = batch.first.second;
					auto n_model_pool = static_cast<D3D12ModelPool*>(model->m_model_pool);

					for (auto& mesh : model->m_meshes)
					{
						auto n_mesh = static_cast<D3D12ModelPool*>(model->m_model_pool)->GetMeshData(mesh.first->id);
						
						std::unordered_map<uint64_t, d3d12::AccelerationStructure>::iterator blas_iterator = data.blasses.find(mesh.first->id);

						if (blas_iterator == data.blasses.end() || n_mesh->data_changed)
						{
							if (n_mesh->data_changed)
							{
								data.old_blasses[data.current_frame_index].push_back((*blas_iterator).second);
								data.blasses.erase(blas_iterator);
								n_mesh->data_changed = false;
							}

							d3d12::Transition(cmd_list, n_model_pool->GetVertexStagingBuffer(), ResourceState::VERTEX_AND_CONSTANT_BUFFER, ResourceState::NON_PIXEL_SHADER_RESOURCE);
							d3d12::Transition(cmd_list, n_model_pool->GetIndexStagingBuffer(), ResourceState::INDEX_BUFFER, ResourceState::NON_PIXEL_SHADER_RESOURCE);

							BuildBLASSingle(device, cmd_list, model, mesh, data);

							d3d12::Transition(cmd_list, n_model_pool->GetVertexStagingBuffer(), ResourceState::NON_PIXEL_SHADER_RESOURCE, ResourceState::VERTEX_AND_CONSTANT_BUFFER);
							d3d12::Transition(cmd_list, n_model_pool->GetIndexStagingBuffer(), ResourceState::NON_PIXEL_SHADER_RESOURCE, ResourceState::INDEX_BUFFER);

							CreateSRVs(cmd_list, data);

							blas_iterator = data.blasses.find(mesh.first->id);
						}

						auto blas = (*blas_iterator).second;

						auto material_id = ExtractMaterialFromMesh(data, mesh.second);

						AppendOffset(data, n_mesh, material_id);

						auto it = batchInfo.find({ model, materials });

						assert(it != batchInfo.end(), "Batch was found in global array, but not in local");

						// Push instances into a array for later use.
						for (uint32_t i = 0U, j = (uint32_t)it->second.num_global_instances; i < j; i++)
						{
							auto transform = batch.second[i].m_model;

							data.out_blas_list.push_back({ blas, offset_id, transform });
						}

						offset_id++;
					}
				}

				d3d12::AccelerationStructure old_accel = data.out_tlas;
				d3d12::UpdateTopLevelAccelerationStructure(data.out_tlas, device, cmd_list, out_heap, data.out_blas_list);

				if (old_accel.m_scratch != data.out_tlas.m_scratch &&
					old_accel.m_native != data.out_tlas.m_native &&
					old_accel.m_instance_desc != data.out_tlas.m_instance_desc)
				{
					data.old_tlas[data.current_frame_index] = old_accel;
				}
			}

		} /* internal */

		inline void ExecuteBuildASTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& scene_graph, RenderTaskHandle handle)
		{
			auto& data = fg.GetData<ASBuildData>(handle);
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto device = n_render_system.m_device;

			data.out_materials_require_update = false;

			data.current_frame_index = n_render_system.GetFrameIdx();

			d3d12::DescriptorHeap* out_heap = cmd_list->m_rt_descriptor_heap->GetHeap();

			// Initialize requirements
			if (data.out_init)
			{
				std::vector<std::shared_ptr<D3D12ModelPool>> model_pools = n_render_system.m_model_pools;
				// Transition all model pools for accel structure creation
				for (auto& pool : model_pools)
				{
					d3d12::Transition(cmd_list, pool->GetVertexStagingBuffer(), ResourceState::VERTEX_AND_CONSTANT_BUFFER, ResourceState::NON_PIXEL_SHADER_RESOURCE);
					d3d12::Transition(cmd_list, pool->GetIndexStagingBuffer(), ResourceState::INDEX_BUFFER, ResourceState::NON_PIXEL_SHADER_RESOURCE);
				}

				// List all materials used by meshes
				internal::BuildBLASList(device, cmd_list, scene_graph, data);

				data.out_tlas = d3d12::CreateTopLevelAccelerationStructure(device, cmd_list, out_heap, data.out_blas_list);
				data.out_tlas.m_native->SetName(L"Highlevelaccel");

				// Transition all model pools back to whatever they were.
				for (auto& pool : model_pools)
				{
					d3d12::Transition(cmd_list, pool->GetVertexStagingBuffer(), ResourceState::NON_PIXEL_SHADER_RESOURCE, ResourceState::VERTEX_AND_CONSTANT_BUFFER);
					d3d12::Transition(cmd_list, pool->GetIndexStagingBuffer(), ResourceState::NON_PIXEL_SHADER_RESOURCE, ResourceState::INDEX_BUFFER);
				}

				internal::CreateSRVs(cmd_list, data);

				data.out_init = false;
			}
			else
			{
				if (data.current_frame_index != data.previous_frame_index)
				{
					for (int i = 0; i < data.old_blasses[data.current_frame_index].size(); ++i)
					{
						d3d12::DestroyAccelerationStructure(data.old_blasses[data.current_frame_index][i]);
					}
					data.old_blasses[data.current_frame_index].clear();

					d3d12::DestroyAccelerationStructure(data.old_tlas[data.current_frame_index]);

					data.old_tlas[data.current_frame_index] = {};
				}

				internal::UpdateTLAS(device, cmd_list, scene_graph, data);
			}

			data.previous_frame_index = n_render_system.GetFrameIdx();
		}

		inline void DestroyBuildASTask(FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			if (!resize)
			{
				auto& data = fg.GetData<ASBuildData>(handle);

				// Small hack to force the allocations to go out of scope, which will tell the allocator to free them
				DescriptorAllocation temp1 = std::move(data.out_scene_ib_alloc);
				DescriptorAllocation temp2 = std::move(data.out_scene_mat_alloc);
				DescriptorAllocation temp3 = std::move(data.out_scene_offset_alloc);
			}
		}

	} /* internal */

	inline void AddBuildAccelerationStructuresTask(FrameGraph& frame_graph)
	{
		std::wstring name(L"Acceleration Structure Builder");

		RenderTargetProperties rt_properties
		{
			RenderTargetProperties::IsRenderWindow(false),
			RenderTargetProperties::Width(std::nullopt),
			RenderTargetProperties::Height(std::nullopt),
			RenderTargetProperties::ExecuteResourceState(ResourceState::UNORDERED_ACCESS),
			RenderTargetProperties::FinishedResourceState(ResourceState::COPY_SOURCE),
			RenderTargetProperties::CreateDSVBuffer(false),
			RenderTargetProperties::DSVFormat(Format::UNKNOWN),
			RenderTargetProperties::RTVFormats({ Format::UNKNOWN }),
			RenderTargetProperties::NumRTVFormats(0),
			RenderTargetProperties::Clear(false),
			RenderTargetProperties::ClearDepth(false),
			RenderTargetProperties::ResourceName(name)
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::SetupBuildASTask(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle) {
			internal::ExecuteBuildASTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::DestroyBuildASTask(fg, handle, resize);
		};

		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		frame_graph.AddTask<ASBuildData>(desc);
	}

} /* wr */
#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "../d3d12/d3d12_structured_buffer_pool.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../engine_registry.hpp"
#include "../util/math.hpp"

#include "../render_tasks/d3d12_deferred_main.hpp"
#include "../imgui_tools.hpp"

namespace wr
{
	struct ASBuildData
	{
		d3d12::DescriptorHeap* out_rt_heap;
		d3d12::AccelerationStructure out_tlas;
		D3D12StructuredBufferHandle* out_sb_material_handle;
		std::vector<std::tuple<d3d12::AccelerationStructure, unsigned int, DirectX::XMMATRIX>> out_blas_list;
		std::vector<temp::RayTracingMaterial_CBData> out_materials;
		std::vector<MaterialHandle*> out_material_handles;
		d3d12::StagingBuffer* out_scene_ib;
		d3d12::StagingBuffer* out_scene_vb;

		std::unordered_map<std::uint64_t, d3d12::AccelerationStructure> blasses;

		bool out_init;
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

			// top level, bottom level and output buffer. (even though I don't use bottom level.)
			d3d12::desc::DescriptorHeapDesc heap_desc;
			heap_desc.m_num_descriptors = 100 + d3d12::settings::num_max_rt_materials; // FIXME: size
			heap_desc.m_type = DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV;
			heap_desc.m_shader_visible = true;
			heap_desc.m_versions = d3d12::settings::num_back_buffers;
			data.out_rt_heap = d3d12::CreateDescriptorHeap(device, heap_desc);
			SetName(data.out_rt_heap, L"Raytracing Task Descriptor Heap");

			// Structured buffer for the materials.
			data.out_sb_material_handle = static_cast<D3D12StructuredBufferHandle*>(n_render_system.m_raytracing_material_sb_pool->Create(sizeof(temp::RayTracingMaterial_CBData) * d3d12::settings::num_max_rt_materials, sizeof(temp::RayTracingMaterial_CBData), false));
		
			// Resize the materials
			data.out_materials.resize(d3d12::settings::num_max_rt_materials);
		}

		namespace internal
		{
			inline void BuildBLASList(d3d12::Device* device, d3d12::CommandList* cmd_list, SceneGraph& scene_graph, ASBuildData& data)
			{
				unsigned int material_id = 0;
				auto batches = scene_graph.GetBatches();

				for (auto& batch : batches)
				{
					auto n_model_pool = static_cast<D3D12ModelPool*>(batch.first->m_model_pool);
					auto vb = n_model_pool->GetVertexStagingBuffer();
					auto ib = n_model_pool->GetIndexStagingBuffer();
					auto model = batch.first;

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
						auto blas = d3d12::CreateBottomLevelAccelerationStructures(device, cmd_list, data.out_rt_heap, { obj });
						cmd_list->m_native->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(blas.m_native));
						blas.m_native->SetName(L"Bottomlevelaccel");

						data.blasses.insert({mesh.first->id, blas});

						data.out_material_handles.push_back(mesh.second); // Used to st eal the textures from the texture pool.

						// Build material
						auto* material_internal = mesh.second->m_pool->GetMaterial(mesh.second->m_id);
						data.out_materials[material_id].idx_offset = n_mesh->m_index_staging_buffer_offset;
						data.out_materials[material_id].vertex_offset = n_mesh->m_vertex_staging_buffer_offset;
						data.out_materials[material_id].albedo_id = material_internal->GetAlbedo().m_id;
						data.out_materials[material_id].normal_id = material_internal->GetNormal().m_id;
						data.out_materials[material_id].roughness_id = material_internal->GetRoughness().m_id;
						data.out_materials[material_id].metallicness_id = material_internal->GetMetallic().m_id;

						// Push instances into a array for later use.
						for (auto i = 0; i < batch.second.num_instances; i++)
						{
							auto transform = batch.second.data.objects[i].m_model;

							data.out_blas_list.push_back({ blas, material_id, transform});
						}

						material_id++;
					}
				}
			}

			inline void UpdateTLAS(d3d12::Device* device, d3d12::CommandList* cmd_list, SceneGraph& scene_graph, ASBuildData& data)
			{
				auto& batches = scene_graph.GetBatches();

				auto prev_size = data.out_blas_list.size();
				data.out_blas_list.clear();
				data.out_blas_list.reserve(prev_size);
				unsigned int material_id = 0;

				// Update transformations
				for (auto& batch : batches)
				{
					auto n_model_pool = static_cast<D3D12ModelPool*>(batch.first->m_model_pool);
					auto model = batch.first;

					for (auto& mesh : model->m_meshes)
					{
						auto n_mesh = static_cast<D3D12ModelPool*>(model->m_model_pool)->GetMeshData(mesh.first->id);

						auto blas = (*data.blasses.find(mesh.first->id)).second;


						// Build material
						auto* material_internal = mesh.second->m_pool->GetMaterial(mesh.second->m_id);
						data.out_materials[material_id].idx_offset = n_mesh->m_index_staging_buffer_offset;
						data.out_materials[material_id].vertex_offset = n_mesh->m_vertex_staging_buffer_offset;
						data.out_materials[material_id].albedo_id = material_internal->GetAlbedo().m_id;
						data.out_materials[material_id].normal_id = material_internal->GetNormal().m_id;
						data.out_materials[material_id].roughness_id = material_internal->GetRoughness().m_id;
						data.out_materials[material_id].metallicness_id = material_internal->GetMetallic().m_id;

						// Push instances into a array for later use.
						for (auto i = 0; i < batch.second.num_instances; i++)
						{
							auto transform = batch.second.data.objects[i].m_model;

							data.out_blas_list.push_back({ 
								blas,
								material_id,
								transform 
							});
						}

						material_id++;
					}
				}

				d3d12::UpdateTopLevelAccelerationStructure(data.out_tlas, device, cmd_list, data.out_rt_heap, data.out_blas_list);
			}

			inline void CreateTextureSRVs(ASBuildData& data)
			{
				for (auto i = 0; i < d3d12::settings::num_back_buffers; i++)
				{
					// Create BYTE ADDRESS buffer view into a staging buffer. Hopefully this works.
					auto& cpu_handle = d3d12::GetCPUHandle(data.out_rt_heap, i);
					d3d12::Offset(cpu_handle, 1, data.out_rt_heap->m_increment_size); // Skip UAV at positon 0
					d3d12::CreateRawSRVFromStagingBuffer(data.out_scene_ib, cpu_handle, 0, data.out_scene_ib->m_size / data.out_scene_ib->m_stride_in_bytes);

					d3d12::Offset(cpu_handle, 1, data.out_rt_heap->m_increment_size); // Skip position 2 since that is for the light buffer.

					// Create material structured buffer view
					d3d12::CreateSRVFromStructuredBuffer(data.out_sb_material_handle->m_native, cpu_handle, 0);

					// Fill descriptor heap with textures used by the scene
					for (auto handle : data.out_material_handles)
					{
						auto* material_internal = handle->m_pool->GetMaterial(handle->m_id);

						auto create_srv = [data, material_internal, i](auto texture_handle)
						{
							auto cpu_handle = d3d12::GetCPUHandle(data.out_rt_heap, i);
							auto* texture_internal = static_cast<wr::d3d12::TextureResource*>(texture_handle.m_pool->GetTexture(texture_handle.m_id));

							d3d12::Offset(cpu_handle, 6 + texture_handle.m_id, data.out_rt_heap->m_increment_size);
							d3d12::CreateSRVFromTexture(texture_internal, cpu_handle);
						};

						create_srv(material_internal->GetAlbedo());
						create_srv(material_internal->GetMetallic());
						create_srv(material_internal->GetNormal());
						create_srv(material_internal->GetRoughness());
					}
				}
			}

		} /* internal */

		inline void ExecuteBuildASTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& scene_graph, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto window = n_render_system.m_window.value();
			auto device = n_render_system.m_device;
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
			auto& data = fg.GetData<ASBuildData>(handle);

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

				data.out_tlas = d3d12::CreateTopLevelAccelerationStructure(device, cmd_list, data.out_rt_heap, data.out_blas_list);
				data.out_tlas.m_native->SetName(L"Highlevelaccel");

				// Transition all model pools back to whatever they were.
				for (auto& pool : model_pools)
				{
					d3d12::Transition(cmd_list, pool->GetVertexStagingBuffer(), ResourceState::NON_PIXEL_SHADER_RESOURCE, ResourceState::VERTEX_AND_CONSTANT_BUFFER);
					d3d12::Transition(cmd_list, pool->GetIndexStagingBuffer(), ResourceState::NON_PIXEL_SHADER_RESOURCE, ResourceState::INDEX_BUFFER);
				}

				internal::CreateTextureSRVs(data);

				data.out_init = false;
			}
			else
			{
				internal::UpdateTLAS(device, cmd_list, scene_graph, data);
			}
		}

		inline void DestroyBuildASTask(FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
		}

	} /* internal */

	inline void AddBuildAccelerationStructuresTask(FrameGraph& frame_graph)
	{
		RenderTargetProperties rt_properties
		{
			false,
			std::nullopt,
			std::nullopt,
			ResourceState::UNORDERED_ACCESS,
			ResourceState::COPY_SOURCE,
			false,
			Format::UNKNOWN,
			{ Format::UNKNOWN },
			0,
			false,
			false
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
		desc.m_name = "Acceleration Structure Builder";
		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		frame_graph.AddTask<ASBuildData>(desc);
	}

} /* wr */

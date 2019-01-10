#include "d3d12_model_pool.hpp"

#include "../util/bitmap_allocator.hpp"

#include "d3d12_functions.hpp"
#include "d3d12_defines.hpp"
#include "d3d12_renderer.hpp"

namespace wr
{

	D3D12ModelPool::D3D12ModelPool(D3D12RenderSystem& render_system,
		std::size_t vertex_buffer_size_in_mb,
		std::size_t index_buffer_size_in_mb) :
		ModelPool(vertex_buffer_size_in_mb, index_buffer_size_in_mb),
		m_render_system(render_system),
		m_current_queue(0)
	{
		m_vertex_buffer = d3d12::CreateStagingBuffer(render_system.m_device,
			nullptr,
			vertex_buffer_size_in_mb * 1024 * 1024,
			sizeof(Vertex),
			ResourceState::VERTEX_AND_CONSTANT_BUFFER);
		SetName(m_vertex_buffer, L"Model Pool Vertex Buffer");
		m_vertex_buffer_size = vertex_buffer_size_in_mb * 1024 * 1024;

		m_index_buffer = d3d12::CreateStagingBuffer(render_system.m_device,
			nullptr,
			index_buffer_size_in_mb * 1024 * 1024,
			sizeof(std::uint32_t),
			ResourceState::INDEX_BUFFER);
		SetName(m_index_buffer, L"Model Pool Index Buffer");
		m_index_buffer_size = index_buffer_size_in_mb * 1024 * 1024;

		m_vertex_heap_start_block = new MemoryBlock;
		m_vertex_heap_start_block->m_free = true;
		m_vertex_heap_start_block->m_next_block = nullptr;
		m_vertex_heap_start_block->m_prev_block = nullptr;
		m_vertex_heap_start_block->m_offset = 0;
		m_vertex_heap_start_block->m_size = m_vertex_buffer_size;

		m_index_heap_start_block = new MemoryBlock;
		m_index_heap_start_block->m_free = true;
		m_index_heap_start_block->m_next_block = nullptr;
		m_index_heap_start_block->m_prev_block = nullptr;
		m_index_heap_start_block->m_offset = 0;
		m_index_heap_start_block->m_size = m_index_buffer_size;

		m_thread_pool = render_system.GetThreadPool();

		m_command_lists.resize(d3d12::settings::max_threads + 2);

		for (auto &list : m_command_lists)
		{
			list = d3d12::CreateCommandList(m_render_system.m_device, 1, wr::CmdListType::CMD_LIST_COPY);
		}

		m_staging_fence = d3d12::CreateFence(m_render_system.m_device);

		m_mesh_stage_queues.resize(d3d12::settings::max_threads);
	}

	D3D12ModelPool::~D3D12ModelPool()
	{
		d3d12::WaitFor(m_staging_fence);
		d3d12::Destroy(m_staging_fence);

		for (auto& handle : m_loaded_meshes)
		{
			static_cast<wr::internal::D3D12MeshInternal*>(handle.second)->m_vertex_upload_finished.get();
			if (static_cast<wr::internal::D3D12MeshInternal*>(handle.second)->m_index_count > 0)
			{
				static_cast<wr::internal::D3D12MeshInternal*>(handle.second)->m_index_upload_finished.get();
			}
			delete handle.second;
		}

		for (auto &list : m_command_lists)
		{
			d3d12::Destroy(list);
		}

		d3d12::Destroy(m_vertex_buffer);
		d3d12::Destroy(m_index_buffer);

		while (m_vertex_heap_start_block != nullptr) 
		{
			MemoryBlock* temp = m_vertex_heap_start_block;
			m_vertex_heap_start_block = m_vertex_heap_start_block->m_next_block;
			delete temp;
		}

		while (m_index_heap_start_block != nullptr)
		{
			MemoryBlock* temp = m_index_heap_start_block;
			m_index_heap_start_block = m_index_heap_start_block->m_next_block;
			delete temp;
		}
	}

	void D3D12ModelPool::Evict()
	{
		d3d12::Evict(m_vertex_buffer);
		d3d12::Evict(m_index_buffer);
	}

	void D3D12ModelPool::MakeResident()
	{
		d3d12::MakeResident(m_vertex_buffer);
		d3d12::MakeResident(m_index_buffer);
	}

	void D3D12ModelPool::StageMeshes()
	{
		bool queues_empty = true;

		for (auto& mesh_queue : m_mesh_stage_queues)
		{
			if (!mesh_queue.empty())
			{
				queues_empty = false;
				break;
			}
		}

		if (queues_empty)
		{
			return;
		}
		
		d3d12::Begin(m_command_lists[0], 0);

		if (m_vertex_buffer->m_is_staged && !m_index_buffer->m_is_staged)
		{
			m_command_lists[0]->m_native->ResourceBarrier(
				1, 
				&CD3DX12_RESOURCE_BARRIER::Transition(
					m_vertex_buffer->m_buffer, 
					(D3D12_RESOURCE_STATES)m_vertex_buffer->m_target_resource_state,
					D3D12_RESOURCE_STATE_COPY_DEST));
		}
		else if(!m_vertex_buffer->m_is_staged && m_index_buffer->m_is_staged)
		{
			m_command_lists[0]->m_native->ResourceBarrier(
				1,
				&CD3DX12_RESOURCE_BARRIER::Transition(
					m_index_buffer->m_buffer,
					(D3D12_RESOURCE_STATES)m_index_buffer->m_target_resource_state,
					D3D12_RESOURCE_STATE_COPY_DEST));
		}
		else if (m_vertex_buffer->m_is_staged && m_index_buffer->m_is_staged)
		{
			std::vector<D3D12_RESOURCE_BARRIER> barriers = {
				CD3DX12_RESOURCE_BARRIER::Transition(
					m_vertex_buffer->m_buffer,
					(D3D12_RESOURCE_STATES)m_vertex_buffer->m_target_resource_state,
					D3D12_RESOURCE_STATE_COPY_DEST) ,
				CD3DX12_RESOURCE_BARRIER::Transition(
					m_index_buffer->m_buffer,
					(D3D12_RESOURCE_STATES)m_index_buffer->m_target_resource_state,
					D3D12_RESOURCE_STATE_COPY_DEST)
			};

			m_command_lists[0]->m_native->ResourceBarrier(2, barriers.data());
		}

		d3d12::End(m_command_lists[0]);

		while (!m_mesh_stage_queue.empty())
		{
			internal::D3D12MeshInternal* d3d12_mesh = static_cast<internal::D3D12MeshInternal*>(m_mesh_stage_queue.front());
			d3d12::StageBufferRegion(m_vertex_buffer,
				d3d12_mesh->m_vertex_staging_buffer_size,
				d3d12_mesh->m_vertex_staging_buffer_offset * d3d12_mesh->m_vertex_staging_buffer_stride,
				cmd_list);
			if (d3d12_mesh->m_index_staging_buffer_size != 0)
			{
				d3d12::StageBufferRegion(m_index_buffer,
					d3d12_mesh->m_index_staging_buffer_size,
					d3d12_mesh->m_index_staging_buffer_offset * sizeof(std::uint32_t),
					cmd_list);
			}
			m_mesh_stage_queue.pop();
		}

		d3d12::Begin(m_command_lists[m_command_lists.size() - 1], 0);

		std::vector<D3D12_RESOURCE_BARRIER> barriers = {
			CD3DX12_RESOURCE_BARRIER::Transition(
				m_vertex_buffer->m_buffer,
				D3D12_RESOURCE_STATE_COPY_DEST,
				(D3D12_RESOURCE_STATES)m_vertex_buffer->m_target_resource_state),
			CD3DX12_RESOURCE_BARRIER::Transition(
				m_index_buffer->m_buffer,
				D3D12_RESOURCE_STATE_COPY_DEST,
				(D3D12_RESOURCE_STATES)m_index_buffer->m_target_resource_state)
		};

		m_command_lists[m_command_lists.size() - 1]->m_native->ResourceBarrier(2, barriers.data());

		d3d12::End(m_command_lists[m_command_lists.size() - 1]);
	}

	d3d12::StagingBuffer * D3D12ModelPool::GetVertexStagingBuffer()
	{
		return m_vertex_buffer;
	}

	d3d12::StagingBuffer * D3D12ModelPool::GetIndexStagingBuffer()
	{
		return m_index_buffer;
	}

	internal::D3D12MeshInternal * D3D12ModelPool::GetMeshData(std::uint64_t mesh_handle)
	{
		return static_cast<internal::D3D12MeshInternal*>(m_loaded_meshes[mesh_handle]);
	}
	
	internal::MeshInternal* D3D12ModelPool::LoadCustom_VerticesAndIndices(void* vertices_data, std::size_t num_vertices, std::size_t vertex_size, void* indices_data, std::size_t num_indices, std::size_t index_size)
	{
		internal::D3D12MeshInternal* mesh = new internal::D3D12MeshInternal();
		memset(mesh, 0, sizeof(internal::D3D12MeshInternal));

		//Allocate vertex buffer memory

		// Find Free Page
		auto vertex_memory_block = AllocateMemory(m_vertex_heap_start_block, num_vertices*vertex_size, vertex_size);

		// Check if we found a page.
		if (vertex_memory_block == nullptr)
		{
			//We haven't found enough pages, so delete the mesh and return a nullptr
			LOGE("Model pool vertex buffer has run out of memory")
			delete mesh;
			return nullptr;
		}
		
		//Repeat the same procedure as before, but now for the index buffer
		auto index_memory_block = AllocateMemory(m_index_heap_start_block, num_indices*index_size, index_size);

		// Check if we found a page.
		if (index_memory_block == nullptr)
		{
			//We haven't found enough pages, so delete the mesh and return a nullptr
			LOGE("Model pool index buffer has run out of memory")
			FreeMemory(m_vertex_heap_start_block, vertex_memory_block);
			delete mesh;
			return nullptr;
		}

		//Store the offset of the allocated memory from the start of the staging buffer
		mesh->m_vertex_staging_buffer_offset = SizeAlign(vertex_memory_block->m_offset, vertex_size) / vertex_size;
		mesh->m_vertex_staging_buffer_size = num_vertices * vertex_size;
		mesh->m_vertex_staging_buffer_stride = vertex_size;
		mesh->m_vertex_count = num_vertices;
		mesh->m_vertex_memory_block = vertex_memory_block;

		mesh->m_vertex_buffer_base_address = m_vertex_buffer->m_gpu_address;

		//Send the vertex data to the vertex staging buffer
		mesh->m_vertex_upload_finished = m_thread_pool->Enqueue(d3d12::UpdateStagingBuffer, 
			m_vertex_buffer, 
			vertices_data,
			num_vertices*vertex_size, 
			mesh->m_vertex_staging_buffer_offset * vertex_size);

		//Store the offset of the allocated memory from the start of the staging buffer
		mesh->m_index_staging_buffer_offset = SizeAlign(index_memory_block->m_offset, index_size) / index_size;
		mesh->m_index_staging_buffer_size = num_indices * index_size;
		mesh->m_index_count = num_indices;
		mesh->m_index_memory_block = index_memory_block;

		mesh->m_index_buffer_base_address = m_index_buffer->m_gpu_address;

		//Send the index data to the index staging buffer
		mesh->m_index_upload_finished = m_thread_pool->Enqueue(d3d12::UpdateStagingBuffer,
			m_index_buffer,
			indices_data,
			num_indices*index_size,
			mesh->m_index_staging_buffer_offset*index_size);
		
		m_mesh_stage_queues[m_current_queue].push(mesh);
		m_current_queue = (m_current_queue + 1) % m_mesh_stage_queues.size();

		return mesh;
	}

	internal::MeshInternal* D3D12ModelPool::LoadCustom_VerticesOnly(void* vertices_data, std::size_t num_vertices, std::size_t vertex_size)
	{
		internal::D3D12MeshInternal* mesh = new internal::D3D12MeshInternal();
		memset(mesh, 0, sizeof(internal::D3D12MeshInternal));

		//Allocate vertex buffer memory
		
		auto vertex_memory_block = AllocateMemory(m_vertex_heap_start_block, num_vertices*vertex_size, vertex_size);

		//The loop has exited, see if we've found enough free pages
		if (vertex_memory_block == nullptr)
		{
			//We haven't found enough pages, so delete the mesh and return a nullptr
			LOGE("Model pool vertex buffer has run out of memory");
			delete mesh;
			return nullptr;
		}

		//Store the offset of the allocated memory from the start of the staging buffer
		mesh->m_vertex_staging_buffer_offset = SizeAlign(vertex_memory_block->m_offset, vertex_size) / vertex_size;
		mesh->m_vertex_staging_buffer_size = num_vertices * vertex_size;
		mesh->m_vertex_staging_buffer_stride = vertex_size;
		mesh->m_vertex_count = num_vertices;
		mesh->m_vertex_memory_block = vertex_memory_block;

		mesh->m_vertex_buffer_base_address = m_vertex_buffer->m_gpu_address;

		//Send the vertex data to the vertex staging buffer
		mesh->m_vertex_upload_finished = m_thread_pool->Enqueue(d3d12::UpdateStagingBuffer,
			m_vertex_buffer,
			vertices_data,
			num_vertices*vertex_size,
			mesh->m_vertex_staging_buffer_offset * vertex_size);
		
		m_mesh_stage_queues[m_current_queue].push(mesh);
		m_current_queue = (m_current_queue + 1) % m_mesh_stage_queues.size();

		return mesh;
	}

	void D3D12ModelPool::DestroyModel(Model * model)
	{
		for (auto& mesh : model->m_meshes)
		{
			DestroyMesh(m_loaded_meshes[mesh.first->id]);
			delete mesh.first;
		}

		//TODO: Destroy possible materials owned by model. Material might be used by multiple models, use ref counts?

		delete model;
	}

	void D3D12ModelPool::DestroyMesh(internal::MeshInternal * mesh)
	{
		//Check for null pointers
		if (mesh == nullptr)
		{
			LOGW("Tried to destroy a mesh that was a nullptr")
			return;
		}
		
		std::map<std::uint64_t, internal::MeshInternal*>::iterator it;
		for (it = m_loaded_meshes.begin(); it != m_loaded_meshes.end(); ++it)
		{
			if (it->second == mesh)
				break;
		}

		if (it != m_loaded_meshes.end())
		{
			FreeID(it->first);

			m_loaded_meshes.erase(it);

			internal::D3D12MeshInternal* n_mesh = static_cast<internal::D3D12MeshInternal*>(mesh);

			n_mesh->m_vertex_upload_finished.get();
			FreeMemory(m_vertex_heap_start_block, static_cast<MemoryBlock*>(n_mesh->m_vertex_memory_block));

			if (n_mesh->m_index_memory_block != nullptr)
			{
				n_mesh->m_index_upload_finished.get();
				FreeMemory(m_index_heap_start_block, static_cast<MemoryBlock*>(n_mesh->m_index_memory_block));
			}

			//Delete the mesh
			delete mesh;
		}
	}

	D3D12ModelPool::MemoryBlock * D3D12ModelPool::AllocateMemory(MemoryBlock * start_block, std::size_t size, std::size_t alignment)
	{
		while (start_block != nullptr) 
		{
			if (start_block->m_size >= size + (start_block->m_offset%alignment == 0 ? 0 : (alignment - start_block->m_offset%alignment)) &&
				start_block->m_free)
			{
				std::size_t needed_size = size + (start_block->m_offset%alignment == 0 ? 0 : (alignment - start_block->m_offset%alignment));

				if (start_block->m_size == needed_size)
				{
					start_block->m_free = false;
					return start_block;
				}
				else
				{
					MemoryBlock* new_block = new MemoryBlock;
					new_block->m_prev_block = start_block;
					new_block->m_next_block = start_block->m_next_block;
					start_block->m_next_block = new_block;
					new_block->m_free = true;
					new_block->m_size = start_block->m_size - needed_size;
					new_block->m_offset = start_block->m_offset + needed_size;

					if (new_block->m_next_block != nullptr) 
					{
						new_block->m_next_block->m_prev_block = new_block;
					}

					start_block->m_free = false;
					start_block->m_size = needed_size;
					return start_block;
				}
			}
			else
			{
				start_block = start_block->m_next_block;
			}
		}
		return nullptr;
	}

	void D3D12ModelPool::FreeMemory(MemoryBlock * heap_start_block, MemoryBlock * block)
	{
		MemoryBlock* heap_block = heap_start_block;
		while (heap_block != nullptr)
		{
			if (heap_block == block) 
			{
				if (heap_block->m_prev_block != nullptr)
				{
					if (heap_block->m_prev_block->m_free)
					{
						heap_block->m_prev_block->m_size += heap_block->m_size;
						if (heap_block->m_next_block != nullptr)
						{
							if (heap_block->m_next_block->m_free)
							{
								heap_block->m_prev_block->m_size += heap_block->m_next_block->m_size;
								heap_block->m_prev_block->m_next_block = heap_block->m_next_block->m_next_block;
								if (heap_block->m_prev_block->m_next_block != nullptr)
								{
									heap_block->m_prev_block->m_next_block->m_prev_block = heap_block->m_prev_block;
								}

								delete heap_block->m_next_block;
								delete heap_block;
								return;
							}
							else
							{
								heap_block->m_next_block->m_prev_block = heap_block->m_prev_block;
								heap_block->m_prev_block->m_next_block = heap_block->m_next_block;

								delete heap_block;
								return;
							}
						}
						else
						{
							heap_block->m_prev_block->m_next_block = heap_block->m_next_block;
							delete heap_block;
							return;
						}
					}
					else
					{
						if (heap_block->m_next_block != nullptr)
						{
							if (heap_block->m_next_block->m_free)
							{
								MemoryBlock* temp = heap_block->m_next_block;
								heap_block->m_size += heap_block->m_next_block->m_size;
								heap_block->m_next_block = heap_block->m_next_block->m_next_block;
								if (heap_block->m_next_block != nullptr)
								{
									heap_block->m_next_block->m_prev_block = heap_block;
								}

								delete temp;
							}
							else
							{
								heap_block->m_free = true;
								return;
							}
						}
						else
						{
							heap_block->m_free = true;
							return;
						}
					}
				}
				else
				{
					if (heap_block->m_next_block != nullptr)
					{
						if (heap_block->m_next_block->m_free)
						{
							MemoryBlock* temp = heap_block->m_next_block;
							heap_block->m_size += heap_block->m_next_block->m_size;
							heap_block->m_next_block = heap_block->m_next_block->m_next_block;
							if (heap_block->m_next_block != nullptr)
							{
								heap_block->m_next_block->m_prev_block = heap_block;
							}

							delete temp;
						}
						else
						{
							heap_block->m_free = true;
							return;
						}
					}
					else
					{
						heap_block->m_free = true;
						return;
					}
				}
			}
			else
			{
				heap_block = heap_block->m_next_block;
			}
		}
	}

} /* wr */
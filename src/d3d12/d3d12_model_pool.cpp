#include "d3d12_model_pool.hpp"

#include "../util/bitmap_allocator.hpp"

#include "d3d12_functions.hpp"
#include "d3d12_defines.hpp"
#include "d3d12_renderer.hpp"

namespace wr
{

	D3D12ModelPool::D3D12ModelPool(D3D12RenderSystem& render_system,
		std::size_t vertex_buffer_size_in_bytes,
		std::size_t index_buffer_size_in_bytes) :
		ModelPool(SizeAlign(vertex_buffer_size_in_bytes, 65536), SizeAlign(index_buffer_size_in_bytes, 65536)),
		m_render_system(render_system)
	{
		m_vertex_buffer = d3d12::CreateStagingBuffer(render_system.m_device,
			nullptr,
			SizeAlign(vertex_buffer_size_in_bytes, 65536),
			sizeof(VertexColor),
			ResourceState::VERTEX_AND_CONSTANT_BUFFER);
		SetName(m_vertex_buffer, L"Model Pool Vertex Buffer");
		m_vertex_buffer_size = SizeAlign(vertex_buffer_size_in_bytes, 65536);

		m_index_buffer = d3d12::CreateStagingBuffer(render_system.m_device,
			nullptr,
			SizeAlign(index_buffer_size_in_bytes, 65536),
			sizeof(std::uint32_t),
			ResourceState::INDEX_BUFFER);
		SetName(m_index_buffer, L"Model Pool Index Buffer");
		m_index_buffer_size = SizeAlign(index_buffer_size_in_bytes, 65536);

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
	}

	D3D12ModelPool::~D3D12ModelPool()
	{
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

		for (auto& handle : m_loaded_meshes)
		{
			delete handle.second;
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

	void D3D12ModelPool::StageMeshes(d3d12::CommandList * cmd_list)
	{
		while (!m_command_queue.empty())
		{
			internal::Command* command = static_cast<internal::Command*>(m_command_queue.front());

			switch (command->m_type)
			{
			case internal::CommandType::STAGE:
			{
				internal::StageCommand* stage_command = static_cast<internal::StageCommand*>(command);
				d3d12::StageBufferRegion(stage_command->m_buffer,
					stage_command->m_size,
					stage_command->m_offset,
					cmd_list);

				delete stage_command;

				m_command_queue.pop();
				break;
			}
			case internal::CommandType::COPY:
			{
				internal::CopyCommand* copy_command = static_cast<internal::CopyCommand*>(command);

				cmd_list->m_native->CopyBufferRegion(copy_command->m_dest,
					copy_command->m_dest_offset,
					copy_command->m_source,
					copy_command->m_source_offset,
					copy_command->m_size);

				delete copy_command;
				m_command_queue.pop();
			}
			break;
			case internal::CommandType::TRANSITION:
			{
				internal::TransitionCommand* transition_command = static_cast<internal::TransitionCommand*>(command);

				cmd_list->m_native->ResourceBarrier(1, 
					&CD3DX12_RESOURCE_BARRIER::Transition(transition_command->m_buffer, 
						static_cast<D3D12_RESOURCE_STATES>(transition_command->m_old_state), 
						static_cast<D3D12_RESOURCE_STATES>(transition_command->m_new_state)));

				delete transition_command;
				m_command_queue.pop();
			}
			break;
			default:
			{
				delete command;
				m_command_queue.pop();
			}
			break;
			}
		}
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

	void D3D12ModelPool::ShrinkToFit()
	{
		ShrinkVertexHeapToFit();
		ShrinkIndexHeapToFit();
	}

	void D3D12ModelPool::ShrinkVertexHeapToFit()
	{
		MemoryBlock* last_occupied_block;
		for (MemoryBlock* mem_block = m_vertex_heap_start_block; mem_block != nullptr; mem_block = mem_block->m_next_block)
		{
			if (mem_block->m_free == false)
			{
				last_occupied_block = mem_block;
			}
		}

		size_t new_size = last_occupied_block->m_offset + last_occupied_block->m_size;
		new_size = SizeAlign(new_size, 65536);

		ID3D12Resource* new_buffer;
		ID3D12Resource* new_staging;

		uint8_t* cpu_address;

		m_render_system.m_device->m_native->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(new_size),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&new_staging));
		NAME_D3D12RESOURCE(new_staging);

		CD3DX12_RANGE read_range(0, 0);

		new_staging->Map(0, &read_range, reinterpret_cast<void**>(&(cpu_address)));

		memcpy(cpu_address, m_vertex_buffer->m_cpu_address, new_size);

		m_vertex_buffer->m_size = new_size;
		m_vertex_buffer->m_is_staged = false;
		m_vertex_buffer->m_is_staged = false;
		SAFE_RELEASE(m_vertex_buffer->m_buffer);
		SAFE_RELEASE(m_vertex_buffer->m_staging);

		m_render_system.m_device->m_native->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(new_size),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&new_buffer));
		NAME_D3D12RESOURCE(new_buffer);

		m_vertex_buffer->m_buffer = new_buffer;
		m_vertex_buffer->m_staging = new_staging;

		internal::StageCommand* stageCommand = new internal::StageCommand;

		stageCommand->m_type = internal::STAGE;
		stageCommand->m_buffer = m_vertex_buffer;
		stageCommand->m_offset = 0;
		stageCommand->m_size = new_size;

		m_command_queue.push(stageCommand);

	}

	void D3D12ModelPool::ShrinkIndexHeapToFit()
	{
		MemoryBlock* last_occupied_block;
		for (MemoryBlock* mem_block = m_index_heap_start_block; mem_block != nullptr; mem_block = mem_block->m_next_block)
		{
			if (mem_block->m_free == false)
			{
				last_occupied_block = mem_block;
			}
		}

		size_t new_size = last_occupied_block->m_offset + last_occupied_block->m_size;
		new_size = SizeAlign(new_size, 65536);

		ID3D12Resource* new_buffer;
		ID3D12Resource* new_staging;

		uint8_t* cpu_address;

		m_render_system.m_device->m_native->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(new_size),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&new_staging));
		NAME_D3D12RESOURCE(new_staging);

		CD3DX12_RANGE read_range(0, 0);

		new_staging->Map(0, &read_range, reinterpret_cast<void**>(&(cpu_address)));

		memcpy(cpu_address, m_index_buffer->m_cpu_address, new_size);

		m_index_buffer->m_size = new_size;
		m_index_buffer->m_is_staged = false;
		m_index_buffer->m_is_staged = false;
		SAFE_RELEASE(m_index_buffer->m_buffer);
		SAFE_RELEASE(m_index_buffer->m_staging);

		m_render_system.m_device->m_native->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(new_size),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&new_buffer));
		NAME_D3D12RESOURCE(new_buffer);

		m_index_buffer->m_buffer = new_buffer;
		m_index_buffer->m_staging = new_staging;

		internal::StageCommand* stageCommand = new internal::StageCommand;

		stageCommand->m_type = internal::STAGE;
		stageCommand->m_buffer = m_index_buffer;
		stageCommand->m_offset = 0;
		stageCommand->m_size = new_size;

		m_command_queue.push(stageCommand);
	}

	void D3D12ModelPool::Defragment()
	{
		DefragmentVertexHeap();
		DefragmentIndexHeap();
	}

	void D3D12ModelPool::DefragmentVertexHeap()
	{
		{
			internal::TransitionCommand* transition_command = new internal::TransitionCommand;
			transition_command->m_type = internal::CommandType::TRANSITION;
			transition_command->m_buffer = m_vertex_buffer->m_buffer;
			if (m_vertex_buffer->m_is_staged)
			{
				transition_command->m_old_state = m_vertex_buffer->m_target_resource_state;
			}
			else
			{
				transition_command->m_old_state = ResourceState::COPY_DEST;
			}
			transition_command->m_new_state = ResourceState::COMMON;

			m_command_queue.push(transition_command);
		}

		MemoryBlock* mem_block = m_vertex_heap_start_block;
		while (mem_block->m_next_block != nullptr)
		{
			if (mem_block->m_free)
			{
				if (mem_block->m_next_block->m_free)
				{
					mem_block->m_size += mem_block->m_next_block->m_size;
					
					mem_block->m_next_block = mem_block->m_next_block->m_next_block;
					delete mem_block->m_next_block->m_prev_block;
					mem_block->m_next_block->m_prev_block = mem_block;
				}
				else
				{
					MemoryBlock* next_block = mem_block->m_next_block;

					size_t original_offset = next_block->m_offset;

					if (mem_block->m_prev_block != nullptr)
					{
						mem_block->m_prev_block->m_next_block = mem_block->m_next_block;
						next_block->m_prev_block = mem_block->m_prev_block;
					}
					else
					{
						m_vertex_heap_start_block = next_block;
						next_block->m_prev_block = nullptr;
					}

					if (next_block->m_next_block != nullptr)
					{
						next_block->m_next_block->m_prev_block = mem_block;
						mem_block->m_next_block = next_block->m_next_block;
					}
					else
					{
						mem_block->m_next_block = nullptr;
					}

					next_block->m_next_block = mem_block;
					mem_block->m_prev_block = next_block;

					next_block->m_offset = SizeAlign(mem_block->m_offset, next_block->m_alignment);
					if (next_block->m_prev_block != nullptr)
					{
						next_block->m_size += next_block->m_offset - mem_block->m_offset;
					}
					mem_block->m_offset = next_block->m_offset + next_block->m_size;
					mem_block->m_size -= next_block->m_offset - mem_block->m_offset;

					std::map<std::uint64_t, internal::MeshInternal*>::iterator it = m_loaded_meshes.begin();

					for (; it != m_loaded_meshes.end(); ++it)
					{
						internal::D3D12MeshInternal* mesh = static_cast<internal::D3D12MeshInternal*>((*it).second);
						if (mesh->m_vertex_memory_block == next_block)
						{
							mesh->m_vertex_staging_buffer_offset = SizeAlign(next_block->m_offset, mesh->m_vertex_staging_buffer_stride)
								/ mesh->m_vertex_staging_buffer_stride;
						}
					}

					internal::CopyCommand* command = new internal::CopyCommand;

					command->m_type = internal::CommandType::COPY;
					command->m_source = m_vertex_buffer->m_buffer;
					command->m_dest = m_vertex_buffer->m_buffer;
					command->m_source_offset = original_offset;
					command->m_dest_offset = next_block->m_offset;
					command->m_size = next_block->m_size;

					m_command_queue.push(command);

					memcpy(m_vertex_buffer->m_cpu_address + next_block->m_offset, m_vertex_buffer->m_cpu_address + original_offset, next_block->m_size);
				}
			}
			else
			{
				mem_block = mem_block->m_next_block;
			}
		}

		{
			internal::TransitionCommand* transition_command = new internal::TransitionCommand;
			transition_command->m_type = internal::CommandType::TRANSITION;
			transition_command->m_buffer = m_vertex_buffer->m_buffer;
			if (m_vertex_buffer->m_is_staged)
			{
				transition_command->m_new_state = m_vertex_buffer->m_target_resource_state;
			}
			else
			{
				transition_command->m_new_state = ResourceState::COPY_DEST;
			}
			transition_command->m_old_state = ResourceState::COMMON;

			m_command_queue.push(transition_command);
		}
	}

	void D3D12ModelPool::DefragmentIndexHeap()
	{
		{
			internal::TransitionCommand* transition_command = new internal::TransitionCommand;
			transition_command->m_type = internal::CommandType::TRANSITION;
			transition_command->m_buffer = m_index_buffer->m_buffer;
			if (m_index_buffer->m_is_staged)
			{
				transition_command->m_old_state = m_index_buffer->m_target_resource_state;
			}
			else
			{
				transition_command->m_old_state = ResourceState::COPY_DEST;
			}
			transition_command->m_new_state = ResourceState::COMMON;

			m_command_queue.push(transition_command);
		}

		MemoryBlock* mem_block = m_index_heap_start_block;
		while (mem_block->m_next_block != nullptr)
		{
			if (mem_block->m_free)
			{
				if (mem_block->m_next_block->m_free)
				{
					mem_block->m_size += mem_block->m_next_block->m_size;

					mem_block->m_next_block = mem_block->m_next_block->m_next_block;
					delete mem_block->m_next_block->m_prev_block;
					mem_block->m_next_block->m_prev_block = mem_block;
				}
				else
				{
					MemoryBlock* next_block = mem_block->m_next_block;

					size_t original_offset = next_block->m_offset;

					if (mem_block->m_prev_block != nullptr)
					{
						mem_block->m_prev_block->m_next_block = mem_block->m_next_block;
						next_block->m_prev_block = mem_block->m_prev_block;
					}
					else
					{
						m_index_heap_start_block = next_block;
						next_block->m_prev_block = nullptr;
					}

					if (next_block->m_next_block != nullptr)
					{
						next_block->m_next_block->m_prev_block = mem_block;
						mem_block->m_next_block = next_block->m_next_block;
					}
					else
					{
						mem_block->m_next_block = nullptr;
					}

					next_block->m_next_block = mem_block;
					mem_block->m_prev_block = next_block;

					next_block->m_offset = SizeAlign(mem_block->m_offset, next_block->m_alignment);
					if (next_block->m_prev_block != nullptr)
					{
						next_block->m_size += next_block->m_offset - mem_block->m_offset;
					}
					mem_block->m_offset = next_block->m_offset + next_block->m_size;
					mem_block->m_size -= next_block->m_offset - mem_block->m_offset;

					std::map<std::uint64_t, internal::MeshInternal*>::iterator it = m_loaded_meshes.begin();

					for (; it != m_loaded_meshes.end(); ++it)
					{
						internal::D3D12MeshInternal* mesh = static_cast<internal::D3D12MeshInternal*>((*it).second);
						if (mesh->m_index_memory_block == next_block)
						{
							mesh->m_index_staging_buffer_offset = SizeAlign(next_block->m_offset, next_block->m_alignment)
								/ next_block->m_alignment;
						}
					}

					internal::CopyCommand* command = new internal::CopyCommand;

					command->m_type = internal::CommandType::COPY;
					command->m_source = m_index_buffer->m_buffer;
					command->m_dest = m_index_buffer->m_buffer;
					command->m_source_offset = original_offset;
					command->m_dest_offset = next_block->m_offset;
					command->m_size = next_block->m_size;

					m_command_queue.push(command);

					memcpy(m_vertex_buffer->m_cpu_address + next_block->m_offset, m_vertex_buffer->m_cpu_address + original_offset, next_block->m_size);
				}
			}
			else
			{
				mem_block = mem_block->m_next_block;
			}
		}

		{
			internal::TransitionCommand* transition_command = new internal::TransitionCommand;
			transition_command->m_type = internal::CommandType::TRANSITION;
			transition_command->m_buffer = m_index_buffer->m_buffer;
			if (m_index_buffer->m_is_staged)
			{
				transition_command->m_new_state = m_index_buffer->m_target_resource_state;
			}
			else
			{
				transition_command->m_new_state = ResourceState::COPY_DEST;
			}
			transition_command->m_old_state = ResourceState::COMMON;

			m_command_queue.push(transition_command);
		}
	}
	
	void D3D12ModelPool::Resize(size_t vertex_heap_new_size, size_t index_heap_new_size)
	{
		// resize vertex heap
		{
			MemoryBlock* last_occupied_block;
			for (MemoryBlock* mem_block = m_vertex_heap_start_block; mem_block != nullptr; mem_block = mem_block->m_next_block)
			{
				if (mem_block->m_free == false)
				{
					last_occupied_block = mem_block;
				}
			}

			size_t new_size = last_occupied_block->m_offset + last_occupied_block->m_size;
			new_size = SizeAlign(new_size, 65536);

			if (new_size >= vertex_heap_new_size)
			{
				ShrinkVertexHeapToFit();
			}
			else
			{
				new_size = SizeAlign(vertex_heap_new_size, 65536);

				ID3D12Resource* new_buffer;
				ID3D12Resource* new_staging;

				uint8_t* cpu_address;

				m_render_system.m_device->m_native->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
					D3D12_HEAP_FLAG_NONE,
					&CD3DX12_RESOURCE_DESC::Buffer(new_size),
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS(&new_staging));
				NAME_D3D12RESOURCE(new_staging);

				CD3DX12_RANGE read_range(0, 0);

				new_staging->Map(0, &read_range, reinterpret_cast<void**>(&(cpu_address)));

				memcpy(cpu_address, m_vertex_buffer->m_cpu_address, new_size);

				m_vertex_buffer->m_size = new_size;
				m_vertex_buffer->m_is_staged = false;
				m_vertex_buffer->m_is_staged = false;
				SAFE_RELEASE(m_vertex_buffer->m_buffer);
				SAFE_RELEASE(m_vertex_buffer->m_staging);

				m_render_system.m_device->m_native->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
					D3D12_HEAP_FLAG_NONE,
					&CD3DX12_RESOURCE_DESC::Buffer(new_size),
					D3D12_RESOURCE_STATE_COPY_DEST,
					nullptr,
					IID_PPV_ARGS(&new_buffer));
				NAME_D3D12RESOURCE(new_buffer);

				m_vertex_buffer->m_buffer = new_buffer;
				m_vertex_buffer->m_staging = new_staging;

				internal::StageCommand* stageCommand = new internal::StageCommand;

				stageCommand->m_type = internal::STAGE;
				stageCommand->m_buffer = m_vertex_buffer;
				stageCommand->m_offset = 0;
				stageCommand->m_size = new_size;

				m_command_queue.push(stageCommand);
			}
		}

		// resize index heap
		{
			MemoryBlock* last_occupied_block;
			for (MemoryBlock* mem_block = m_index_heap_start_block; mem_block != nullptr; mem_block = mem_block->m_next_block)
			{
				if (mem_block->m_free == false)
				{
					last_occupied_block = mem_block;
				}
			}

			size_t new_size = last_occupied_block->m_offset + last_occupied_block->m_size;
			new_size = SizeAlign(new_size, 65536);

			if (new_size >= index_heap_new_size)
			{
				ShrinkIndexHeapToFit();
			}
			else
			{
				new_size = SizeAlign(index_heap_new_size, 65536);

				ID3D12Resource* new_buffer;
				ID3D12Resource* new_staging;

				uint8_t* cpu_address;

				m_render_system.m_device->m_native->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
					D3D12_HEAP_FLAG_NONE,
					&CD3DX12_RESOURCE_DESC::Buffer(new_size),
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS(&new_staging));
				NAME_D3D12RESOURCE(new_staging);

				CD3DX12_RANGE read_range(0, 0);

				new_staging->Map(0, &read_range, reinterpret_cast<void**>(&(cpu_address)));

				memcpy(cpu_address, m_index_buffer->m_cpu_address, new_size);

				m_index_buffer->m_size = new_size;
				m_index_buffer->m_is_staged = false;
				m_index_buffer->m_is_staged = false;
				SAFE_RELEASE(m_index_buffer->m_buffer);
				SAFE_RELEASE(m_index_buffer->m_staging);

				m_render_system.m_device->m_native->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
					D3D12_HEAP_FLAG_NONE,
					&CD3DX12_RESOURCE_DESC::Buffer(new_size),
					D3D12_RESOURCE_STATE_COPY_DEST,
					nullptr,
					IID_PPV_ARGS(&new_buffer));
				NAME_D3D12RESOURCE(new_buffer);

				m_index_buffer->m_buffer = new_buffer;
				m_index_buffer->m_staging = new_staging;

				internal::StageCommand* stageCommand = new internal::StageCommand;

				stageCommand->m_type = internal::STAGE;
				stageCommand->m_buffer = m_index_buffer;
				stageCommand->m_offset = 0;
				stageCommand->m_size = new_size;

				m_command_queue.push(stageCommand);
			}
		}
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
		d3d12::UpdateStagingBuffer(m_vertex_buffer, vertices_data, num_vertices*vertex_size, mesh->m_vertex_staging_buffer_offset * vertex_size);

		//Store the offset of the allocated memory from the start of the staging buffer
		mesh->m_index_staging_buffer_offset = SizeAlign(index_memory_block->m_offset, index_size) / index_size;
		mesh->m_index_staging_buffer_size = num_indices * index_size;
		mesh->m_index_count = num_indices;
		mesh->m_index_memory_block = index_memory_block;

		mesh->m_index_buffer_base_address = m_index_buffer->m_gpu_address;

		//Send the index data to the index staging buffer
		d3d12::UpdateStagingBuffer(m_index_buffer, indices_data, num_indices*index_size, mesh->m_index_staging_buffer_offset * index_size);
		
		internal::StageCommand* vertex_command = new internal::StageCommand;
		vertex_command->m_type = internal::CommandType::STAGE;
		vertex_command->m_buffer = m_vertex_buffer;
		vertex_command->m_offset = mesh->m_vertex_staging_buffer_offset * mesh->m_vertex_staging_buffer_stride;
		vertex_command->m_size = mesh->m_vertex_staging_buffer_size;

		m_command_queue.push(vertex_command);
		
		internal::StageCommand* index_command = new internal::StageCommand;
		index_command->m_type = internal::CommandType::STAGE;
		index_command->m_buffer = m_index_buffer;
		index_command->m_offset = mesh->m_index_staging_buffer_offset * index_size;
		index_command->m_size = mesh->m_index_staging_buffer_size;

		m_command_queue.push(index_command);

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
		d3d12::UpdateStagingBuffer(m_vertex_buffer, vertices_data, num_vertices*vertex_size, mesh->m_vertex_staging_buffer_offset);

		internal::StageCommand* vertex_command = new internal::StageCommand;
		vertex_command->m_type = internal::CommandType::STAGE;
		vertex_command->m_buffer = m_vertex_buffer;
		vertex_command->m_offset = mesh->m_vertex_staging_buffer_offset * mesh->m_vertex_staging_buffer_stride;
		vertex_command->m_size = mesh->m_vertex_staging_buffer_size;

		m_command_queue.push(vertex_command);

		return mesh;
	}

	void D3D12ModelPool::UpdateMeshData(Mesh * mesh, void * vertices_data, std::size_t num_vertices, std::size_t vertex_size, void * indices_data, std::size_t num_indices, std::size_t index_size)
	{
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

			FreeMemory(m_vertex_heap_start_block, static_cast<MemoryBlock*>(n_mesh->m_vertex_memory_block));

			if (n_mesh->m_index_memory_block != nullptr)
			{
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
					start_block->m_alignment = alignment;
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
					start_block->m_alignment = alignment;
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

	size_t D3D12ModelPool::GetVertexHeapOccupiedSpace()
	{
		MemoryBlock* mem_block = m_vertex_heap_start_block;
		size_t size = 0;
		while (mem_block != nullptr)
		{
			if (!mem_block->m_free)
			{
				size += mem_block->m_size;
			}
			mem_block = mem_block->m_next_block;
		}
		return size;
	}

	size_t D3D12ModelPool::GetIndexHeapOccupiedSpace()
	{
		MemoryBlock* mem_block = m_index_heap_start_block;
		size_t size = 0;
		while (mem_block != nullptr)
		{
			if (!mem_block->m_free)
			{
				size += mem_block->m_size;
			}
			mem_block = mem_block->m_next_block;
		}
		return size;
	}

	size_t D3D12ModelPool::GetVertexHeapFreeSpace()
	{
		MemoryBlock* mem_block = m_vertex_heap_start_block;
		size_t size = 0;
		while (mem_block != nullptr)
		{
			if (mem_block->m_free)
			{
				size += mem_block->m_size;
			}
			mem_block = mem_block->m_next_block;
		}
		return size;
	}

	size_t D3D12ModelPool::GetIndexHeapFreeSpace()
	{
		MemoryBlock* mem_block = m_index_heap_start_block;
		size_t size = 0;
		while (mem_block != nullptr)
		{
			if (mem_block->m_free)
			{
				size += mem_block->m_size;
			}
			mem_block = mem_block->m_next_block;
		}
		return size;
	}

	size_t D3D12ModelPool::GetVertexHeapSize()
	{
		MemoryBlock* mem_block = m_vertex_heap_start_block;
		size_t size = 0;
		while (mem_block != nullptr)
		{
			size += mem_block->m_size;
			mem_block = mem_block->m_next_block;
		}
		return size;
	}

	size_t D3D12ModelPool::GetIndexHeapSize()
	{
		MemoryBlock* mem_block = m_index_heap_start_block;
		size_t size = 0;
		while (mem_block != nullptr)
		{
			size += mem_block->m_size;
			mem_block = mem_block->m_next_block;
		}
		return size;
	}

} /* wr */
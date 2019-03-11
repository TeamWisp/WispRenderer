#pragma once

#include "../model_pool.hpp"
#include "d3d12_structs.hpp"

#include <queue>

namespace wr::d3d12
{
	struct HeapResource;
	struct StagingBuffer;
}

namespace wr
{

	class D3D12RenderSystem;

	namespace internal
	{
		struct D3D12MeshInternal : MeshInternal
		{
			D3D12_GPU_VIRTUAL_ADDRESS m_vertex_buffer_base_address;
			std::size_t m_vertex_staging_buffer_offset;
			std::size_t m_vertex_staging_buffer_size;
			std::size_t m_vertex_staging_buffer_stride;
			std::size_t m_vertex_count;
			void* m_vertex_memory_block;
			D3D12_GPU_VIRTUAL_ADDRESS m_index_buffer_base_address;
			std::size_t m_index_staging_buffer_offset;
			std::size_t m_index_count_offset;
			std::size_t m_index_staging_buffer_size;
			std::size_t m_index_count;
			void* m_index_memory_block;
		};

		enum CommandType
		{
			STAGE,
			COPY,
			READ,
			TRANSITION,
		};

		struct Command
		{
			CommandType m_type;
		};

		struct StageCommand : Command
		{
			d3d12::StagingBuffer* m_buffer;
			std::size_t m_size;
			std::size_t m_offset;
		};

		struct CopyCommand : Command
		{
			ID3D12Resource* m_source;
			ID3D12Resource* m_dest;
			std::size_t m_size;
			std::size_t m_source_offset;
			std::size_t m_dest_offset;
		};

		struct ReadCommand : Command
		{
			d3d12::StagingBuffer* m_buffer;
			std::size_t m_size;
			std::size_t m_offset;
		};

		struct TransitionCommand : Command
		{
			ID3D12Resource* m_buffer;
			ResourceState m_old_state;
			ResourceState m_new_state;
		};
	}

	class D3D12ModelPool : public ModelPool
	{
	public:
		explicit D3D12ModelPool(D3D12RenderSystem& render_system,
			std::size_t vertex_buffer_pool_size_in_mb,
			std::size_t index_buffer_pool_size_in_mb);
		~D3D12ModelPool() final;

		void Evict() final;
		void MakeResident() final;

		void StageMeshes(d3d12::CommandList* cmd_list);

		d3d12::StagingBuffer* GetVertexStagingBuffer();
		d3d12::StagingBuffer* GetIndexStagingBuffer();

		internal::D3D12MeshInternal* GetMeshData(std::uint64_t mesh_handle);

		// Shrinks down both heaps to the minimum size required. 
		// Does not rearrange the contents of the heaps, meaning that it doesn't shrink to the absolute minimum size.
		// To do that, call Defragment first.
		void ShrinkToFit() final;
		void ShrinkVertexHeapToFit() final;
		void ShrinkIndexHeapToFit() final;

		// Removes any holes in the memory, stitching all allocations back together to maximize the amount of contiguous free space.
		// These functions are called automatically if the allocator has enough free space but no large enough free blocks.
		void Defragment() final;
		void DefragmentVertexHeap() final;
		void DefragmentIndexHeap() final;

		size_t GetVertexHeapOccupiedSpace() final;
		size_t GetIndexHeapOccupiedSpace() final;

		size_t GetVertexHeapFreeSpace() final;
		size_t GetIndexHeapFreeSpace() final;

		size_t GetVertexHeapSize() final;
		size_t GetIndexHeapSize() final;

		// Resizes both heaps to the supplied sizes. 
		// If the supplied size is smaller than the required size the heaps will resize to the required size instead.
		void Resize(size_t vertex_heap_new_size, size_t index_heap_new_size) final;
		void ResizeVertexHeap(size_t vertex_heap_new_size) final;
		void ResizeIndexHeap(size_t index_heap_new_size) final;

		// Returns if the model pool data has been changed. Use this to see if additional data needs to be altered.
		bool IsUpdated() { return m_updated; };

		void SetUpdated(bool updated) { m_updated = updated; };

	private:
		internal::MeshInternal* LoadCustom_VerticesAndIndices(void* vertices_data, std::size_t num_vertices, std::size_t vertex_size, void* indices_data, std::size_t num_indices, std::size_t index_size) final;
		internal::MeshInternal* LoadCustom_VerticesOnly(void* vertices_data, std::size_t num_vertices, std::size_t vertex_size) final;

		virtual void UpdateMeshData(Mesh* mesh, void* vertices_data, std::size_t num_vertices, std::size_t vertex_size, void* indices_data, std::size_t num_indices, std::size_t index_size) final;

		void UpdateMeshVertexData(Mesh* mesh, void* vertices_data, std::size_t num_vertices, std::size_t vertex_size);
		void UpdateMeshIndexData(Mesh* mesh, void* indices_data, std::size_t num_indices, std::size_t indices_size);

		void DestroyModel(Model* model) final;
		void DestroyMesh(internal::MeshInternal* mesh) final;

		d3d12::StagingBuffer* m_vertex_buffer;
		d3d12::StagingBuffer* m_index_buffer;

		struct MemoryBlock
		{
			MemoryBlock* m_prev_block;
			MemoryBlock* m_next_block;
			std::size_t m_size;
			std::size_t m_offset;
			std::size_t m_alignment;
			bool m_free;
		};

		MemoryBlock* m_vertex_heap_start_block;
		MemoryBlock* m_index_heap_start_block;

		MemoryBlock* AllocateMemory(MemoryBlock* start_block, std::size_t size, std::size_t alignment);
		void FreeMemory(MemoryBlock* heap_start_block,  MemoryBlock* block);
		
		std::queue<internal::Command*> m_command_queue;

		ID3D12Resource* m_intermediate_buffer;
		std::size_t m_intermediate_size;

		std::uint64_t m_vertex_buffer_size;
		std::uint64_t m_index_buffer_size;

		bool m_updated;

		D3D12RenderSystem& m_render_system;
	};
} /* wr::d3d12 */
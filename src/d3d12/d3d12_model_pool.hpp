#pragma once

#include "../model_pool.hpp"
#include "d3d12_structs.hpp"

#include "../util/thread_pool.hpp"

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
			std::future<void> m_vertex_upload_finished;
			D3D12_GPU_VIRTUAL_ADDRESS m_index_buffer_base_address;
			std::size_t m_index_staging_buffer_offset;
			std::size_t m_index_count_offset;
			std::size_t m_index_staging_buffer_size;
			std::size_t m_index_count;
			void* m_index_memory_block;
			std::future<void> m_index_upload_finished;
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

		void StageMeshes();

		void WaitForStaging();

		d3d12::StagingBuffer* GetVertexStagingBuffer();
		d3d12::StagingBuffer* GetIndexStagingBuffer();

		internal::D3D12MeshInternal* GetMeshData(std::uint64_t mesh_handle);

	private:
		internal::MeshInternal* LoadCustom_VerticesAndIndices(void* vertices_data, std::size_t num_vertices, std::size_t vertex_size, void* indices_data, std::size_t num_indices, std::size_t index_size) final;
		internal::MeshInternal* LoadCustom_VerticesOnly(void* vertices_data, std::size_t num_vertices, std::size_t vertex_size) final;

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
			bool m_free;
		};

		MemoryBlock* m_vertex_heap_start_block;
		MemoryBlock* m_index_heap_start_block;

		MemoryBlock* AllocateMemory(MemoryBlock* start_block, std::size_t size, std::size_t alignment);
		void FreeMemory(MemoryBlock* heap_start_block,  MemoryBlock* block);
		
		std::vector<std::queue<internal::MeshInternal*>> m_mesh_stage_queues;

		std::uint32_t m_current_queue;

		std::uint64_t m_vertex_buffer_size;
		std::uint64_t m_index_buffer_size;

		util::ThreadPool* m_thread_pool;

		std::vector<d3d12::CommandList*> m_command_lists;
		std::vector<d3d12::CommandList*> m_transition_command_lists;

		std::vector<std::future<void>> m_command_recording_futures;
		std::vector<bool> m_command_recording_launched;

		d3d12::Fence* m_staging_fence;
		d3d12::Fence* m_transition_fence;

		bool m_waiting_for_staging;

		D3D12RenderSystem& m_render_system;
	};
} /* wr::d3d12 */
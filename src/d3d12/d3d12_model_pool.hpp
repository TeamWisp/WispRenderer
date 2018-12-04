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

	struct D3D12Mesh : Mesh
	{
		D3D12_GPU_VIRTUAL_ADDRESS m_vertex_buffer_base_address;
		std::size_t m_vertex_staging_buffer_offset;
		std::size_t m_vertex_staging_buffer_size;
		std::size_t m_vertex_staging_buffer_stride;
		std::size_t m_vertex_count;
		D3D12_GPU_VIRTUAL_ADDRESS m_index_buffer_base_address;
		std::size_t m_index_staging_buffer_offset;
		std::size_t m_index_count_offset;
		std::size_t m_index_staging_buffer_size;
		std::size_t m_index_count;
	};

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

	private:
		Model* LoadFBX(std::string_view path, ModelType type) final;
		Mesh* LoadCustom_VerticesAndIndices(void* vertices_data, std::size_t num_vertices, std::size_t vertex_size, void* indices_data, std::size_t num_indices, std::size_t index_size) final;
		Mesh* LoadCustom_VerticesOnly(void* vertices_data, std::size_t num_vertices, std::size_t vertex_size) final;

		void DestroyModel(Model* model) final;
		void DestroyMesh(Mesh* mesh) final;

		d3d12::StagingBuffer* m_vertex_buffer;
		d3d12::StagingBuffer* m_index_buffer;

		std::vector<std::uint64_t> m_vertex_buffer_bitmap;
		std::vector<std::uint64_t> m_index_buffer_bitmap;

		std::vector<Mesh*> m_mesh_handles;

		std::queue<Mesh*> m_mesh_stage_queue;

		std::uint64_t m_vertex_buffer_size;
		std::uint64_t m_index_buffer_size;

		D3D12RenderSystem& m_render_system;
	};
} /* wr::d3d12 */
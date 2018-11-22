#pragma once

#include "../resource_pool_model.hpp"

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
		d3d12::StagingBuffer* m_vertex_buffer;
	};

	class D3D12ModelPool : public ModelPool
	{
	public:
		explicit D3D12ModelPool(D3D12RenderSystem& render_system, std::size_t size_in_mb);
		~D3D12ModelPool() final;

		void Evict() final;
		void MakeResident() final;

	protected:
		Model* LoadFBX(std::string_view path, ModelType type) final;
		Mesh* LoadCustom_VerticesAndIndices(void* vertices_data, std::size_t num_vertices, std::size_t vertex_size, void* indices_data, std::size_t num_indices, std::size_t index_size) final;
		Mesh* LoadCustom_VerticesOnly(void* vertices_data, std::size_t num_vertices, std::size_t vertex_size) final;

		D3D12RenderSystem& m_render_system;
	};
} /* wr::d3d12 */
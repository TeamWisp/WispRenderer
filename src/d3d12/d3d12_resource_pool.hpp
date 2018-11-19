#pragma once

#include "../resource_pool.hpp"

namespace d3d12
{
	struct HeapResource;
	struct StagingBuffer;
}

namespace wr
{

	class D3D12RenderSystem;

	struct D3D12TextureHandle : TextureHandle { };

	struct D3D12MaterialHandle : MaterialHandle { };

	struct D3D12ConstantBufferHandle : ConstantBufferHandle {
		d3d12::HeapResource* m_native;
	};

	struct D3D12Mesh : Mesh
	{
		d3d12::StagingBuffer* m_vertex_buffer;
	};

	class D3D12MaterialPool : public MaterialPool
	{
	public:
		explicit D3D12MaterialPool(std::size_t size_in_mb);
		virtual ~D3D12MaterialPool() final;
		
		virtual void Evict() final;
		virtual void MakeResident() final;
	
	protected:
		virtual MaterialHandle LoadMaterial(std::string_view path) final;
		virtual TextureHandle LoadPNG(std::string_view path) final;
		virtual TextureHandle LoadDDS(std::string_view path) final;
		virtual TextureHandle LoadHDR(std::string_view path) final;
	};

	class D3D12ModelPool : public ModelPool
	{
	public:
		explicit D3D12ModelPool(D3D12RenderSystem& render_system, std::size_t size_in_mb);
		virtual ~D3D12ModelPool() final;

		virtual void Evict() final;
		virtual void MakeResident() final;

	protected:
		virtual Model* LoadFBX(std::string_view path, ModelType type) final;
		virtual Mesh* LoadCustom_VerticesAndIndices(void* vertices_data, std::size_t num_vertices, std::size_t vertex_size, void* indices_data, std::size_t num_indices, std::size_t index_size) final;
		virtual Mesh* LoadCustom_VerticesOnly(void* vertices_data, std::size_t num_vertices, std::size_t vertex_size) final;

		D3D12RenderSystem& m_render_system;
	};
}
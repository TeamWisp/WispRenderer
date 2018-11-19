#include "d3d12_resource_pool.hpp"

#include "d3d12_functions.hpp"
#include "d3d12_renderer.hpp"

namespace wr
{

	D3D12MaterialPool::D3D12MaterialPool(std::size_t size_in_mb) : MaterialPool(size_in_mb)
	{
	}

	D3D12MaterialPool::~D3D12MaterialPool()
	{
	}

	void D3D12MaterialPool::Evict()
	{
	}

	void D3D12MaterialPool::MakeResident()
	{
	}

	MaterialHandle D3D12MaterialPool::LoadMaterial(std::string_view path)
	{
		return MaterialHandle();
	}

	TextureHandle D3D12MaterialPool::LoadPNG(std::string_view path)
	{
		return TextureHandle();
	}

	TextureHandle D3D12MaterialPool::LoadDDS(std::string_view path)
	{
		return TextureHandle();
	}

	TextureHandle D3D12MaterialPool::LoadHDR(std::string_view path)
	{
		return TextureHandle();
	}

	D3D12ModelPool::D3D12ModelPool(D3D12RenderSystem& render_system, std::size_t size_in_mb) : ModelPool(size_in_mb), m_render_system(render_system)
	{
	}

	D3D12ModelPool::~D3D12ModelPool()
	{
	}

	void D3D12ModelPool::Evict()
	{
	}

	void D3D12ModelPool::MakeResident()
	{
	}

	Model* D3D12ModelPool::LoadFBX(std::string_view path, ModelType type)
	{
		return new Model();
	}

	Mesh* D3D12ModelPool::LoadCustom_VerticesAndIndices(void* vertices_data, std::size_t num_vertices, std::size_t vertex_size, void* indices_data, std::size_t num_indices, std::size_t index_size)
	{
		return new Mesh();
	}

	Mesh* D3D12ModelPool::LoadCustom_VerticesOnly(void* vertices_data, std::size_t num_vertices, std::size_t vertex_size)
	{
		auto mesh = new D3D12Mesh();
		auto device = m_render_system.m_device;

		mesh->m_vertex_buffer = d3d12::CreateStagingBuffer(device, vertices_data, vertex_size * num_vertices, vertex_size, d3d12::ResourceState::VERTEX_AND_CONSTANT_BUFFER);

		return mesh;
	}

} /* wr */

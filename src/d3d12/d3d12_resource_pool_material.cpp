#include "d3d12_resource_pool_material.hpp"

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

	MaterialHandle* D3D12MaterialPool::LoadMaterial(std::string_view path)
	{
		return new MaterialHandle();
	}

	TextureHandle* D3D12MaterialPool::LoadPNG(std::string_view path)
	{
		return new TextureHandle();
	}

	TextureHandle* D3D12MaterialPool::LoadDDS(std::string_view path)
	{
		return new TextureHandle();
	}

	TextureHandle* D3D12MaterialPool::LoadHDR(std::string_view path)
	{
		return new TextureHandle();
	}

}
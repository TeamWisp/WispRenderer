#include "d3d12_resource_pool.hpp"

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

	D3D12ModelPool::D3D12ModelPool(std::size_t size_in_mb) : ModelPool(size_in_mb)
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

	ModelHandle D3D12ModelPool::LoadFBX(std::string_view path, ModelType type)
	{
		return ModelHandle();
	}

} /* wr */

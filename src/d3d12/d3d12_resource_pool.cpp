#include "d3d12_resource_pool.hpp"

wr::D3D12MaterialPool::D3D12MaterialPool(std::size_t size_in_mb) : MaterialPool(size_in_mb)
{
}

wr::D3D12MaterialPool::~D3D12MaterialPool()
{
}

void wr::D3D12MaterialPool::Evict()
{
}

void wr::D3D12MaterialPool::MakeResident()
{
}

wr::MaterialHandle wr::D3D12MaterialPool::LoadMaterial(std::string_view path)
{
	return MaterialHandle();
}

wr::TextureHandle wr::D3D12MaterialPool::LoadPNG(std::string_view path)
{
	return TextureHandle();
}

wr::TextureHandle wr::D3D12MaterialPool::LoadDDS(std::string_view path)
{
	return TextureHandle();
}

wr::TextureHandle wr::D3D12MaterialPool::LoadHDR(std::string_view path)
{
	return TextureHandle();
}

wr::D3D12ModelPool::D3D12ModelPool(std::size_t size_in_mb) : ModelPool(size_in_mb)
{
}

wr::D3D12ModelPool::~D3D12ModelPool()
{
}

void wr::D3D12ModelPool::Evict()
{
}

void wr::D3D12ModelPool::MakeResident()
{
}

wr::ModelHandle wr::D3D12ModelPool::LoadFBX(std::string_view path, ModelType type)
{
	return ModelHandle();
}

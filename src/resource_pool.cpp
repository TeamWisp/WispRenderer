#include "resource_pool.hpp"

wr::MaterialPool::MaterialPool(std::size_t size_in_mb)
{
}

wr::MaterialPool::~MaterialPool()
{
}

wr::MaterialHandle wr::MaterialPool::Load(std::string_view path, TextureType type)
{
	return MaterialHandle();
}

wr::ModelPool::ModelPool(std::size_t size_in_mb)
{
}

wr::ModelPool::~ModelPool()
{
}

wr::ModelHandle wr::ModelPool::Load(std::string_view path, ModelType type)
{
	return wr::ModelHandle();
}

std::pair<wr::ModelHandle, std::vector<wr::MaterialHandle>> wr::ModelPool::LoadWithMaterials(wr::MaterialPool* material_pool, std::string_view path, ModelType type)
{
	return std::pair<ModelHandle, std::vector<MaterialHandle>>();
}
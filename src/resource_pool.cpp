#include "resource_pool.hpp"

namespace wr
{

	MaterialPool::MaterialPool(std::size_t size_in_mb) : m_size_in_mb(size_in_mb)
	{
	}

	//! Loads a material
	MaterialHandle MaterialPool::Load(std::string_view path, TextureType type)
	{
		return MaterialHandle();
	}

	ModelPool::ModelPool(std::size_t size_in_mb) : m_size_in_mb(size_in_mb)
	{
	}

	//! Loads a model without materials
	ModelHandle ModelPool::Load(std::string_view path, ModelType type)
	{
		return ModelHandle();
	}

	//! Loads a model with materials
	std::pair<ModelHandle, std::vector<MaterialHandle>> ModelPool::LoadWithMaterials(MaterialPool* material_pool, std::string_view path, ModelType type)
	{
		return std::pair<ModelHandle, std::vector<MaterialHandle>>();
	}

} /* wr */
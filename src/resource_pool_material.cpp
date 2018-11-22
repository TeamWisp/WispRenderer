#include "resource_pool_material.hpp"

namespace wr {
	MaterialPool::MaterialPool(std::size_t size_in_mb) : m_size_in_mb(size_in_mb)
	{
	}

	//! Loads a material
	MaterialHandle* MaterialPool::Load(std::string_view path, TextureType type)
	{
		return LoadMaterial(path);
	}
} /* wr */
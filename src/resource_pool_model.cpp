#include "resource_pool_model.hpp"

namespace wr {
	ModelPool::ModelPool(std::size_t size_in_mb) : m_size_in_mb(size_in_mb)
	{
	}

	//! Loads a model without materials
	Model* ModelPool::Load(std::string_view path, ModelType type)
	{
		return new Model();
	}

	//! Loads a model with materials
	std::pair<Model*, std::vector<MaterialHandle>> ModelPool::LoadWithMaterials(MaterialPool* material_pool, std::string_view path, ModelType type)
	{
		auto model = new Model();
		return std::pair<Model*, std::vector<MaterialHandle>>();
	}
} /* wr */
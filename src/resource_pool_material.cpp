#define NOMINMAX

#include "resource_pool_material.hpp"
#include "resource_pool_texture.hpp"

#include <assimp/material.h>

namespace wr 
{
	MaterialPool::MaterialPool()
	{
	}

	MaterialHandle MaterialPool::Create()
	{
		MaterialHandle handle;
		handle.m_pool = this;
		handle.m_id = m_id_factory.GetUnusedID();

		Material* mat = new Material();

		m_materials.insert(std::make_pair(handle.m_id, mat));

		return handle;
	}

	MaterialHandle MaterialPool::Create(TextureHandle& albedo, TextureHandle& normal, 
										TextureHandle& rough_met, TextureHandle& ao, 
										bool is_alpha_masked, bool is_double_sided)
	{
		MaterialHandle handle;
		handle.m_pool = this;
		handle.m_id = m_id_factory.GetUnusedID();

		Material* mat = new Material();

		mat->m_albedo = albedo;
		mat->m_normal = normal;
		mat->m_rough_metallic = rough_met;
		mat->m_ao = ao;
		mat->m_alpha_masked = is_alpha_masked;
		mat->m_double_sided = is_double_sided;

		m_materials.insert(std::make_pair(handle.m_id, mat));

		return handle;
	}

	MaterialHandle* MaterialPool::Load(aiMaterial* material)
	{
		return nullptr;
	}

	Material* MaterialPool::GetMaterial(uint64_t material_id)
	{
		return m_materials[material_id];
	}

} /* wr */
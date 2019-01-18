#define NOMINMAX

#include "resource_pool_texture.hpp"
#include "material_pool.hpp"
#include "util/log.hpp"

#include <assimp/material.h>

namespace wr 
{
	//////////////////////////////
	//	  MATERIAL FUNCTIONS	//
	//////////////////////////////

	Material::Material()
	{
		//TODO: Initialize to default textures, something like Unreal's checkerboard pattern
	}

	Material::Material(TextureHandle albedo,
					   TextureHandle normal,
					   TextureHandle roughness,
					   TextureHandle metallic,
		               TextureHandle ao,
		               bool alpha_masked,
					   bool double_sided)
	{
		m_texture_pool = albedo.m_pool;

		SetAlbedo(albedo);
		SetNormal(normal);
		SetRoughness(roughness);
		SetMetallic(metallic);
		SetAmbientOcclusion(ao);
	}

	void Material::SetAlbedo(TextureHandle albedo)
	{
		if (albedo.m_pool != m_texture_pool
			&& m_texture_pool)
		{
			LOGW("Textures in a material need to belong to the same texture pool");
			m_albedo = m_texture_pool->GetDefaultAlbedo();
		}
		else
		{
			m_albedo = albedo;
		}
	}


	void Material::SetNormal(TextureHandle normal)
	{
		if (normal.m_pool != m_texture_pool
			&& m_texture_pool)
		{
			m_normal = m_texture_pool->GetDefaultNormal();
			LOGW("Textures in a material need to belong to the same texture pool");
		}
		else
		{
			m_normal = normal;
		}
	}

	void Material::SetRoughness(TextureHandle roughness)
	{
		if (roughness.m_pool != m_texture_pool
			&& m_texture_pool)
		{
			m_rougness = m_texture_pool->GetDefaultRoughness();
			LOGW("Textures in a material need to belong to the same texture pool");
		}
		else
		{
			m_rougness = roughness;
		}
	}

	void Material::SetMetallic(TextureHandle metallic)
	{
		if (metallic.m_pool != m_texture_pool
			&& m_texture_pool)
		{
			m_metallic = m_texture_pool->GetDefaultMetalic();
			LOGW("Textures in a material need to belong to the same texture pool");
		}
		else
		{
			m_metallic = metallic;
		}
	}

	void Material::SetAmbientOcclusion(TextureHandle ao)
	{
		if (ao.m_pool != m_texture_pool
			&& m_texture_pool)
		{
			m_ao = m_texture_pool->GetDefaultAO();
			LOGW("Textures in a material need to belong to the same texture pool");
		}
		else
		{
			m_ao = ao;
		}
	}



	//////////////////////////////
	//	MATERIAL POOL FUNCTIONS	//
	//////////////////////////////

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

	MaterialHandle* MaterialPool::Create(TextureHandle& albedo, TextureHandle& normal,
										TextureHandle& roughness, TextureHandle& metallic,
										TextureHandle& ao, bool is_alpha_masked, bool is_double_sided)
	{
		MaterialHandle* handle = new MaterialHandle();
		handle->m_pool = this;
		handle->m_id = m_id_factory.GetUnusedID();

		Material* mat = new Material(albedo, normal, roughness, metallic, ao, is_alpha_masked, is_double_sided);

		m_materials.insert(std::make_pair(handle->m_id, mat));

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
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
					   TextureHandle rough_met,
		               TextureHandle ao,
		               bool alpha_masked,
					   bool double_sided)
	{
		m_texture_pool = albedo.m_pool;

		if (m_texture_pool == normal.m_pool && 
			m_texture_pool == rough_met.m_pool && 
			m_texture_pool == ao.m_pool)
		{
			m_albedo = albedo;
			m_normal = normal;
			m_rough_metallic = rough_met;
			m_ao = ao;
			m_alpha_masked = alpha_masked;
			m_double_sided = double_sided;
		}
		else
		{
			LOGC("Textures in a material need to belong to the same texture pool");
		}
	}

	void Material::SetAlbedo(TextureHandle albedo)
	{
		if (albedo.m_pool != m_texture_pool
			&& m_texture_pool)
		{
			LOGC("Textures in a material need to belong to the same texture pool");
		}

		m_albedo = albedo;
	}


	void Material::SetNormal(TextureHandle normal)
	{
		if (normal.m_pool != m_texture_pool
			&& m_texture_pool)
		{
			LOGC("Textures in a material need to belong to the same texture pool");
		}

		m_normal = normal;
	}

	void Material::SetRoughMetallic(TextureHandle rough_met)
	{
		if (rough_met.m_pool != m_texture_pool
			&& m_texture_pool)
		{
			LOGC("Textures in a material need to belong to the same texture pool");
		}

		m_rough_metallic = rough_met;
	}

	void Material::SetAmbientOcclusion(TextureHandle ao)
	{
		if (ao.m_pool != m_texture_pool
			&& m_texture_pool)
		{
			LOGC("Textures in a material need to belong to the same texture pool");
		}

		m_ao = ao;
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

	MaterialHandle MaterialPool::Create(TextureHandle& albedo, TextureHandle& normal, 
										TextureHandle& rough_met, TextureHandle& ao, 
										bool is_alpha_masked, bool is_double_sided)
	{
		MaterialHandle handle;
		handle.m_pool = this;
		handle.m_id = m_id_factory.GetUnusedID();

		Material* mat = new Material(albedo, normal, rough_met, ao, is_alpha_masked, is_double_sided);

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
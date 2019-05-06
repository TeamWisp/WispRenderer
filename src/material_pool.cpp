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
		memset(&m_material_data, 0, sizeof(m_material_data));

		m_albedo = { nullptr,0 };
		m_normal = { nullptr,0 };
		m_rougness = { nullptr,0 };
		m_metallic = { nullptr,0 };
		m_ao = { nullptr,0 };
		m_emissive = { nullptr,0 };
	}

	Material::Material(TextureHandle albedo,
					   TextureHandle normal,
					   TextureHandle roughness,
					   TextureHandle metallic,
		               TextureHandle ao,
					   TextureHandle emissive,
		               bool alpha_masked,
					   bool double_sided)
	{
		m_texture_pool = albedo.m_pool;
	
		
		memset(&m_material_data, 0, sizeof(m_material_data));

		SetAlbedo(albedo);
		SetNormal(normal);
		SetRoughness(roughness);
		SetMetallic(metallic);
		SetAmbientOcclusion(ao);
		SetEmissive(emissive);

		SetAlphaMasked(alpha_masked);
		SetDoubleSided(double_sided);
	}

	Material::~Material()
	{
		m_constant_buffer_handle->m_pool->Destroy(m_constant_buffer_handle);
	}

	void Material::SetAlbedo(TextureHandle albedo)
	{
		if (albedo.m_pool != m_texture_pool
			&& m_texture_pool)
		{
			LOGW("Textures in a material need to belong to the same texture pool");
			m_albedo = m_texture_pool->GetDefaultAlbedo();
			m_material_data.m_material_flags.m_has_albedo_texture = false;
		}
		else
		{
			if (!m_texture_pool)
			{
				m_texture_pool = albedo.m_pool;
			}
			m_albedo = albedo;
			m_material_data.m_material_flags.m_has_albedo_texture = true;
			
		}
	}

	void Material::UseAlbedoTexture(bool use_albedo)
	{
		m_material_data.m_material_flags.m_has_albedo_texture = use_albedo;
	}


	void Material::SetNormal(TextureHandle normal)
	{
		if (normal.m_pool != m_texture_pool
			&& m_texture_pool)
		{
			m_normal = m_texture_pool->GetDefaultNormal();
			LOGW("Textures in a material need to belong to the same texture pool");
			m_material_data.m_material_flags.m_has_normal_texture = false;
		}
		else
		{
			if (!m_texture_pool)
			{
				m_texture_pool = normal.m_pool;
			}
			m_normal = normal;
			m_material_data.m_material_flags.m_has_normal_texture = true;
			
		}
	}

	void Material::UseNormalTexture(bool use_normal)
	{
		m_material_data.m_material_flags.m_has_normal_texture = use_normal;
	}

	void Material::SetRoughness(TextureHandle roughness)
	{
		if (roughness.m_pool != m_texture_pool
			&& m_texture_pool)
		{
			m_rougness = m_texture_pool->GetDefaultRoughness();
			LOGW("Textures in a material need to belong to the same texture pool");
			m_material_data.m_material_flags.m_has_roughness_texture = false;
		}
		else
		{
			if (!m_texture_pool)
			{
				m_texture_pool = roughness.m_pool;
			}
			m_rougness = roughness;
			m_material_data.m_material_flags.m_has_roughness_texture = true;
		}
	}

	void Material::UseRoughnessTexture(bool use_roughness)
	{
		m_material_data.m_material_flags.m_has_roughness_texture = use_roughness;
	}

	void Material::SetMetallic(TextureHandle metallic)
	{
		if (metallic.m_pool != m_texture_pool
			&& m_texture_pool)
		{
			m_metallic = m_texture_pool->GetDefaultMetalic();
			LOGW("Textures in a material need to belong to the same texture pool");
			m_material_data.m_material_flags.m_has_metallic_texture = false;
		}
		else
		{
			if (!m_texture_pool)
			{
				m_texture_pool = metallic.m_pool;
			}
			m_metallic = metallic;
			m_material_data.m_material_flags.m_has_metallic_texture = true;
		}
	}

	void Material::UseMetallicTexture(bool use_metallic)
	{
		m_material_data.m_material_flags.m_has_metallic_texture = use_metallic;
	}

	void Material::SetAmbientOcclusion(TextureHandle ao)
	{
		if (ao.m_pool != m_texture_pool
			&& m_texture_pool)
		{
			m_ao = m_texture_pool->GetDefaultAO();
			LOGW("Textures in a material need to belong to the same texture pool");
			m_material_data.m_material_flags.m_has_ao_texture = false;
		}
		else
		{
			if (!m_texture_pool)
			{
				m_texture_pool = ao.m_pool;
			}
			m_ao = ao;
			m_material_data.m_material_flags.m_has_ao_texture = true;
		}
	}

	void Material::UseAOTexture(bool use_ao)
	{
		m_material_data.m_material_flags.m_has_ao_texture = use_ao;
	}

	void Material::SetEmissive(TextureHandle emissive)
	{
		if (emissive.m_pool != m_texture_pool
			&& m_texture_pool)
		{
			m_emissive = m_texture_pool->GetDefaultEmissive();
			LOGW("Textures in a material need to belong to the same texture pool");
			m_material_data.m_material_flags.m_has_emissive_texture = false;
		}
		else
		{
			if (!m_texture_pool)
			{
				m_texture_pool = emissive.m_pool;
			}
			m_emissive = emissive;
			m_material_data.m_material_flags.m_has_emissive_texture = true;
		}
	}

	void Material::UseEmissiveTexture(bool use_emissive)
	{
		m_material_data.m_material_flags.m_has_emissive_texture = use_emissive;
	}

	void Material::SetConstantAlbedo(DirectX::XMFLOAT3 color)
	{
		m_material_data.m_color = DirectX::XMVectorSet(color.x, color.y, color.z, GetConstantAlpha());
		m_material_data.m_material_flags.m_has_albedo_constant = true;
	}

	void Material::SetConstantAlpha(float alpha)
	{
		m_material_data.m_color = DirectX::XMVectorSetW(m_material_data.m_color, alpha);
		m_material_data.m_material_flags.m_has_constant_alpha = true;
	}

	void Material::SetConstantMetallic(DirectX::XMFLOAT3 metallic)
	{
		m_material_data.m_metallic_roughness = DirectX::XMVectorSet(metallic.x, metallic.y, metallic.z, GetConstantRoughness());
		m_material_data.m_material_flags.m_has_metallic_constant = true;
	}

	void Material::SetConstantRoughness(float roughness)
	{
		m_material_data.m_metallic_roughness = DirectX::XMVectorSetW(m_material_data.m_metallic_roughness, roughness);
		m_material_data.m_material_flags.m_has_roughness_constant = true;
	}

	void Material::SetAlphaMasked(bool alpha_masked)
	{
		m_alpha_masked = alpha_masked;
		m_material_data.m_material_flags.m_has_alpha_mask = alpha_masked;
	}

	void Material::SetDoubleSided(bool double_sided)
	{
		m_double_sided = double_sided;
		m_material_data.m_material_flags.m_is_double_sided = double_sided;
	}

	void Material::UpdateConstantBuffer()
	{
		m_constant_buffer_handle->m_pool->Update(
			m_constant_buffer_handle,
			sizeof(MaterialData),
			0,
			reinterpret_cast<std::uint8_t*>(&m_material_data));
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
		mat->SetConstantBufferHandle(m_constant_buffer_pool->Create(sizeof(Material::MaterialData)));

		m_materials.insert(std::make_pair(handle.m_id, mat));

		return handle;
	}

	MaterialHandle MaterialPool::Create(TextureHandle& albedo, TextureHandle& normal,
										TextureHandle& roughness, TextureHandle& metallic,
										TextureHandle& ao, TextureHandle& emissive, bool is_alpha_masked, bool is_double_sided)
	{
		MaterialHandle handle = {};
		handle.m_pool = this;
		handle.m_id = m_id_factory.GetUnusedID();

		Material* mat = new Material(albedo, normal, roughness, metallic, ao, emissive, is_alpha_masked, is_double_sided);
		mat->SetConstantBufferHandle(m_constant_buffer_pool->Create(sizeof(Material::MaterialData)));
		mat->UpdateConstantBuffer();

		m_materials.insert(std::make_pair(handle.m_id, mat));

		return handle;
	}

	Material* MaterialPool::GetMaterial(MaterialHandle handle)
	{
		// Return the material if available.
		if (auto it = m_materials.find(handle.m_id); it != m_materials.end())
		{
			return it->second;
		}

		LOGE("Failed to obtain a material from pool.");
		return nullptr;
	}

	bool MaterialPool::HasMaterial(MaterialHandle handle) const
	{
		return m_materials.find(handle.m_id) != m_materials.end();
	}

} /* wr */
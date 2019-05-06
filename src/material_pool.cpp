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

	Material::Material(TexturePool* pool): m_texture_pool(pool)
	{
		memset(&m_material_data, 0, sizeof(m_material_data));
	}

	Material::Material(TexturePool *pool,
					   TextureHandle albedo,
					   TextureHandle normal,
					   TextureHandle roughness,
					   TextureHandle metallic,
		               TextureHandle ao,
		               float alpha_constant,
					   float double_sided): Material(pool)
	{

		SetTexture(MaterialTextureType::ALBEDO, albedo);
		SetTexture(MaterialTextureType::NORMAL, normal);
		SetTexture(MaterialTextureType::ROUGHNESS, roughness);
		SetTexture(MaterialTextureType::METALLIC, metallic);
		SetTexture(MaterialTextureType::AO, ao);

		SetConstant(MaterialConstantType::USE_ALPHA_CONSTANT, alpha_constant);
		SetConstant(MaterialConstantType::IS_DOUBLE_SIDED, double_sided);
	}

	TextureHandle Material::GetTexture(MaterialTextureType type) { 
		return m_textures[size_t(type) % size_t(MaterialTextureType::COUNT)]; 
	}

	void Material::SetTexture(MaterialTextureType type, TextureHandle handle)
	{
		if (handle.m_pool != m_texture_pool || !m_texture_pool)
		{
			ClearTexture(type);
			LOGW("Textures in a material need to belong to the same texture pool");
		}
		else
		{
			m_textures[size_t(type) % size_t(MaterialTextureType::COUNT)] = handle;
			m_material_data.m_material_flags.m_value = uint32_t(m_material_data.m_material_flags.m_value) | (1 << uint32_t(type));
		}
	}


	void Material::ClearTexture(MaterialTextureType type) {
		m_textures[size_t(type) % size_t(MaterialTextureType::COUNT)] = { nullptr, 0 };
		m_material_data.m_material_flags.m_value = uint32_t(m_material_data.m_material_flags.m_value) & (~(1 << uint32_t(type)));
	}

	bool Material::HasTexture(MaterialTextureType type) {
		return m_textures[size_t(type) % size_t(MaterialTextureType::COUNT)].m_pool;
	}

	void Material::SetConstant(MaterialConstantType type, float val) {
		float arr[1] = { val };
		SetConstant(type, arr);
	}

	void Material::GetConstant(MaterialConstantType type, float &val) {
		float arr[1];
		GetConstant(type, arr);
		val = *arr;
	}

	Material::~Material()
	{
		m_constant_buffer_handle->m_pool->Destroy(m_constant_buffer_handle);
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

	MaterialHandle MaterialPool::Create(TexturePool* pool)
	{
		MaterialHandle handle;
		handle.m_pool = this;
		handle.m_id = m_id_factory.GetUnusedID();

		Material* mat = new Material(pool);
		mat->SetConstantBufferHandle(m_constant_buffer_pool->Create(sizeof(Material::MaterialData)));

		m_materials.insert(std::make_pair(handle.m_id, mat));

		return handle;
	}

	MaterialHandle MaterialPool::Create(TexturePool* pool, TextureHandle& albedo, TextureHandle& normal,
										TextureHandle& roughness, TextureHandle& metallic,
										TextureHandle& ao, bool is_alpha_masked, bool is_double_sided)
	{
		MaterialHandle handle = {};
		handle.m_pool = this;
		handle.m_id = m_id_factory.GetUnusedID();

		Material* mat = new Material(pool, albedo, normal, roughness, metallic, ao, is_alpha_masked, is_double_sided);
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

	void MaterialPool::DestroyMaterial(MaterialHandle handle)
	{
		auto it = m_materials.find(handle.m_id);

		if (it == m_materials.end())
			LOGC("Can't destroy material; it's not part of the material pool");

		auto *ptr = it->second;
		m_materials.erase(it);
		delete ptr;

	}

	bool MaterialPool::HasMaterial(MaterialHandle handle) const
	{
		return m_materials.find(handle.m_id) != m_materials.end();
	}

} /* wr */
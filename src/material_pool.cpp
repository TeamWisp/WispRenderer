// Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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

		SetConstant<MaterialConstant::EMISSIVE_MULTIPLIER>(1.0f);
		SetConstant<MaterialConstant::ALBEDO_UV_SCALE>(1.0f);
		SetConstant<MaterialConstant::NORMAL_UV_SCALE>(1.0f);
		SetConstant<MaterialConstant::ROUGHNESS_UV_SCALE>(1.0f);
		SetConstant<MaterialConstant::METALLIC_UV_SCALE>(1.0f);
		SetConstant<MaterialConstant::EMISSIVE_UV_SCALE>(1.0f);
		SetConstant<MaterialConstant::AO_UV_SCALE>(1.0f);
	}

	Material::Material(TexturePool *pool,
					   TextureHandle albedo,
					   TextureHandle normal,
					   TextureHandle roughness,
					   TextureHandle metallic,
					   TextureHandle emissive,
		               TextureHandle ao,
					   MaterialUVScales scales,
		               bool alpha_masked,
					   bool double_sided): Material(pool)
	{

		SetTexture(TextureType::ALBEDO, albedo);
		SetTexture(TextureType::NORMAL, normal);
		SetTexture(TextureType::ROUGHNESS, roughness);
		SetTexture(TextureType::METALLIC, metallic);
		SetTexture(TextureType::EMISSIVE, emissive);
		SetTexture(TextureType::AO, ao);

		m_material_data.albedo_scale = scales.m_albedo_scale;
		m_material_data.normal_scale = scales.m_normal_scale;
		m_material_data.roughness_scale = scales.m_roughness_scale;
		m_material_data.metallic_scale = scales.m_metallic_scale;
		m_material_data.emissive_scale = scales.m_emissive_scale;
		m_material_data.ao_scale = scales.m_ao_scale;

		SetConstant<MaterialConstant::ALBEDO_UV_SCALE>(scales.m_albedo_scale);
		SetConstant<MaterialConstant::NORMAL_UV_SCALE>(scales.m_normal_scale);
		SetConstant<MaterialConstant::ROUGHNESS_UV_SCALE>(scales.m_roughness_scale);
		SetConstant<MaterialConstant::METALLIC_UV_SCALE>(scales.m_metallic_scale);
		SetConstant<MaterialConstant::EMISSIVE_UV_SCALE>(scales.m_emissive_scale);
		SetConstant<MaterialConstant::AO_UV_SCALE>(scales.m_ao_scale);
		SetConstant<MaterialConstant::IS_ALPHA_MASKED>(alpha_masked);
		SetConstant<MaterialConstant::IS_DOUBLE_SIDED>(double_sided);
		SetConstant<MaterialConstant::EMISSIVE_MULTIPLIER>(1.0f);
	}

	TextureHandle Material::GetTexture(TextureType type) { 
		return m_textures[size_t(type) % size_t(TextureType::COUNT)]; 
	}

	void Material::SetTexture(TextureType type, TextureHandle handle)
	{
		if (handle.m_pool != m_texture_pool || !m_texture_pool)
		{
			ClearTexture(type);
			//LOGW("Textures in a material need to belong to the same texture pool");
		}
		else
		{
			m_textures[size_t(type) % size_t(TextureType::COUNT)] = handle;
			m_material_data.m_material_flags.m_value = uint32_t(m_material_data.m_material_flags.m_value) | (1 << uint32_t(type));
		}
	}


	void Material::ClearTexture(TextureType type) {
		m_textures[size_t(type) % size_t(TextureType::COUNT)] = { nullptr, 0 };
		m_material_data.m_material_flags.m_value = uint32_t(m_material_data.m_material_flags.m_value) & (~(1 << uint32_t(type)));
	}

	bool Material::HasTexture(TextureType type) {
		return m_textures[size_t(type) % size_t(TextureType::COUNT)].m_pool;
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
										TextureHandle& roughness, TextureHandle& metallic, TextureHandle& emissive,
										TextureHandle& ao, MaterialUVScales& mat_scales, bool is_alpha_masked, bool is_double_sided)
	{
		MaterialHandle handle = {};
		handle.m_pool = this;
		handle.m_id = m_id_factory.GetUnusedID();

		Material* mat = new Material(pool, albedo, normal, roughness, metallic, emissive, ao, mat_scales, is_alpha_masked, is_double_sided);
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
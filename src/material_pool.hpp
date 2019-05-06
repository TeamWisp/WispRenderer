#pragma once

#include <string_view>
#include <unordered_map>
#include <vector>
#include <optional>
#include <stdint.h>
#include <d3d12.h>
#include <DirectXMath.h>

#include "structs.hpp"
#include "util/defines.hpp"
#include "util/log.hpp"
#include "id_factory.hpp"
#include "constant_buffer_pool.hpp"

struct aiMaterial;

namespace wr
{
	enum class MaterialTextureType : size_t
	{
		ALBEDO,
		NORMAL,
		ROUGHNESS,
		METALLIC,
		AO,
		COUNT
	};

	//u32 type = { u16 offset (in floats), u16 size (in floats) }
	enum class MaterialConstantType : uint32_t
	{
		COLOR = (0 << 16) | 3,
		R = (0 << 16) | 1, 
		G = (1 << 16) | 1,
		B = (2 << 16) | 1,
		METALLIC = (3 << 16) | 1,
		ROUGHNESS = (4 << 16) | 1,
		IS_DOUBLE_SIDED = (5 << 16) | 1,
		USE_ALPHA_CONSTANT = (6 << 16) | 1,
		MAX_OFFSET = 7
	};

	class Material
	{
	public:

		Material(TexturePool* pool);
		Material(TexturePool* pool,
				 TextureHandle albedo, 
				 TextureHandle normal, 
				 TextureHandle roughness,
				 TextureHandle metallic,
				 TextureHandle ao, 
				 float alpha_constant = false,
				 float double_sided = false);

		Material(const Material& rhs) = default;

		~Material();

		TextureHandle GetTexture(MaterialTextureType type);
		void SetTexture(MaterialTextureType type, TextureHandle handle);
		void ClearTexture(MaterialTextureType type);
		bool HasTexture(MaterialTextureType type);

		template<uint16_t s>
		void GetConstant(MaterialConstantType type, float(&val)[s]);
		
		template<uint16_t s>
		void SetConstant(MaterialConstantType type, float(&val)[s]);

		void SetConstant(MaterialConstantType type, float val);
		void GetConstant(MaterialConstantType type, float &val);

		TexturePool* const GetTexturePool() { return m_texture_pool; }

		ConstantBufferHandle* const GetConstantBufferHandle() const { return m_constant_buffer_handle; };
		void SetConstantBufferHandle(ConstantBufferHandle* handle) { m_constant_buffer_handle = handle; };

		void UpdateConstantBuffer();

		union TextureFlags
		{
			struct {
				uint32_t m_has_albedo_texture : 1;
				uint32_t m_has_normal_texture : 1;
				uint32_t m_has_roughness_texture : 1;
				uint32_t m_has_metallic_texture : 1;
				uint32_t m_has_ao_texture : 1;
			};

			uint32_t m_value;
		};

		union MaterialData
		{
			struct {

				float m_color[3];
				float m_metallic;

				float m_roughness;
				float m_is_double_sided;
				float m_use_alpha_constant;
				TextureFlags m_material_flags;

			};

			float m_constant_data[size_t(MaterialConstantType::MAX_OFFSET) + 1]{};

		};

		MaterialData GetMaterialData() const { return m_material_data; }

	protected:

		union {

			TextureHandle m_textures[size_t(MaterialTextureType::COUNT)]{};

			struct {
				TextureHandle m_albedo;
				TextureHandle m_normal;
				TextureHandle m_roughness;
				TextureHandle m_metallic;
				TextureHandle m_ao;
			};

		};

		TexturePool* m_texture_pool = nullptr;

		ConstantBufferHandle* m_constant_buffer_handle;

		MaterialData m_material_data;
	};

	class MaterialPool
	{
	public:
		explicit MaterialPool();
		virtual ~MaterialPool() = default;

		MaterialPool(MaterialPool const &) = delete;
		MaterialPool& operator=(MaterialPool const &) = delete;
		MaterialPool(MaterialPool&&) = delete;
		MaterialPool& operator=(MaterialPool&&) = delete;

		//Creates an empty material. The user is responsible of filling in the texture handles.
		//TODO: Give Materials default textures
		[[nodiscard]] MaterialHandle Create(TexturePool* pool);
		[[nodiscard]] MaterialHandle Create(TexturePool* pool,
											TextureHandle& albedo,
											TextureHandle& normal,
											TextureHandle& roughness,
											TextureHandle& metallic,
											TextureHandle& ao, 
											bool is_alpha_masked = false, 
											bool is_double_sided = false);



		virtual void Evict() {}
		virtual void MakeResident() {}

		/*! Obtain a material from the pool. */
		/*!
			Throws an error if no material was found.
		*/
		virtual Material* GetMaterial(MaterialHandle handle);
		/*! Check if the material owns a material with the specified handle */
		bool HasMaterial(MaterialHandle handle) const;

		void DestroyMaterial(MaterialHandle handle);

	protected:
		std::shared_ptr<ConstantBufferPool> m_constant_buffer_pool;

		std::unordered_map<uint64_t, Material*> m_materials;

		IDFactory m_id_factory;
	};

	//Material template functions

	template<uint16_t s>
	void Material::GetConstant(MaterialConstantType type, float(&val)[s])
	{
		uint16_t offset = uint16_t(uint32_t(type) >> 16) % uint16_t(MaterialConstantType::MAX_OFFSET);
		uint16_t count = uint16_t(type);

		if (s > count || offset + s > uint16_t(MaterialConstantType::MAX_OFFSET))
			LOGC("Material constant out of bounds");

		memcpy(val, m_material_data.m_constant_data + offset, count * sizeof(float));
	}

	template<uint16_t s>
	void Material::SetConstant(MaterialConstantType type, float(&val)[s])
	{
		uint16_t offset = uint16_t(uint32_t(type) >> 16) % uint16_t(MaterialConstantType::MAX_OFFSET);
		uint16_t count = uint16_t(type);

		if (s > count || offset + s > uint16_t(MaterialConstantType::MAX_OFFSET))
			LOGC("Material constant out of bounds");

		memcpy(m_material_data.m_constant_data + offset, val, count * sizeof(float));
	}

} /* wr */
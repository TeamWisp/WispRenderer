#pragma once

#include <string_view>
#include <unordered_map>
#include <vector>
#include <optional>
#include <stdint.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <memory>
#include <array>

#include "structs.hpp"
#include "util/defines.hpp"
#include "util/log.hpp"
#include "id_factory.hpp"
#include "constant_buffer_pool.hpp"

struct aiMaterial;

namespace wr
{
	enum class TextureType : size_t
	{
		ALBEDO = 0,
		NORMAL,
		ROUGHNESS,
		METALLIC,
		EMISSIVE,
		AO,
		COUNT
	};

	//u32 type = { u16 offset (in floats), u16 isArray (0 = a constant, 1 = an array) }
	enum class MaterialConstant : uint32_t
	{
		COLOR = (0 << 16) | 1,
		R = (0 << 16) | 0,
		G = (1 << 16) | 0,
		B = (2 << 16) | 0,
		METALLIC = (3 << 16) | 0,
		ROUGHNESS = (4 << 16) | 0,
		EMISSIVE_MULTIPLIER = (5 << 16) | 0,
		IS_DOUBLE_SIDED = (6 << 16) | 0,
		IS_ALPHA_MASKED = (7 << 16) | 0,
		ALBEDO_UV_SCALE = (8 << 16) | 0,
		NORMAL_UV_SCALE = (9 << 16) | 0,
		ROUGHNESS_UV_SCALE = (10 << 16) | 0,
		METALLIC_UV_SCALE = (11 << 16) | 0,
		EMISSIVE_UV_SCALE = (12 << 16) | 0,
		AO_UV_SCALE = (13 << 16) | 0,
		MAX_OFFSET = 14
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
				 TextureHandle emissive,
				 TextureHandle ao, 
				 MaterialUVScales mat_scales,
				 bool alpha_masked = false,
				 bool double_sided = false);

		Material(const Material& rhs) = default;

		~Material();

		TextureHandle GetTexture(TextureType type);
		void SetTexture(TextureType type, TextureHandle handle);
		void ClearTexture(TextureType type);
		bool HasTexture(TextureType type);

		template<MaterialConstant type>
		typename std::enable_if<uint16_t(type) == 0, float>::type GetConstant() {
			return m_material_data.m_constant_data[uint32_t(type) >> 16];
		}

		template<MaterialConstant type>
		typename std::enable_if<uint16_t(type) != 0, std::array<float, 3>>::type GetConstant() {
			float* arr = m_material_data.m_constant_data;
			uint32_t i = uint32_t(type) >> 16;
			return { arr[i], arr[i + 1], arr[i + 2] };
		}

		template<MaterialConstant type>
		void SetConstant(typename std::enable_if<uint16_t(type) == 0, float>::type x) {
			m_material_data.m_constant_data[uint32_t(type) >> 16] = x;
		}

		template<MaterialConstant type>
		void SetConstant(const typename std::enable_if<uint16_t(type) != 0, std::array<float, 3>>::type& val) {
			float* arr = m_material_data.m_constant_data;
			uint32_t i = uint32_t(type) >> 16;
			memcpy(arr, val.data(), sizeof(val));
		}

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
				uint32_t m_has_emissive_texture : 1;
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
				float m_emissive_multiplier;
				float m_is_double_sided;
				float m_use_alpha_constant;

				float albedo_scale;
				float normal_scale;
				float roughness_scale;
				float metallic_scale;

				float emissive_scale;
				float ao_scale;
				float m_padding;
				TextureFlags m_material_flags;

			};

			float m_constant_data[size_t(MaterialConstant::MAX_OFFSET)]{};

		};

		MaterialData GetMaterialData() const { return m_material_data; }

	protected:

		union {

			TextureHandle m_textures[size_t(TextureType::COUNT)]{};

			struct {
				TextureHandle m_albedo;
				TextureHandle m_normal;
				TextureHandle m_roughness;
				TextureHandle m_metallic;
				TextureHandle m_emissive;
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
		virtual ~MaterialPool();

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
											TextureHandle& emissive,
											TextureHandle& ao,
											MaterialUVScales& mat_scales,
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


} /* wr */
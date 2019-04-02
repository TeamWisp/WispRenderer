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
#include "id_factory.hpp"
#include "constant_buffer_pool.hpp"

struct aiMaterial;

namespace wr
{
	enum class MaterialPBR : unsigned int
	{
		ALBEDO = 0,
		NORMAL,
		ROUGHNESS,
		METALLIC,

		COUNT
	};

	class Material
	{
	public:

		Material();
		Material(TextureHandle albedo, 
				 TextureHandle normal, 
				 TextureHandle roughness,
				 TextureHandle metallic,
				 TextureHandle ao, 
				 bool alpha_masked = false, 
				 bool double_sided = false);

		Material(const Material& rhs) = default;

		~Material();

		TextureHandle GetAlbedo() { return m_albedo; }
		void SetAlbedo(TextureHandle albedo);

		void UseAlbedoTexture(bool use_albedo);

		TextureHandle GetNormal() { return m_normal; }
		void SetNormal(TextureHandle normal);

		void UseNormalTexture(bool use_normal);

		TextureHandle GetRoughness() { return m_rougness; }
		void SetRoughness(TextureHandle roughness);

		void UseRoughnessTexture(bool use_roughness);

		TextureHandle GetMetallic() { return m_metallic; }
		void SetMetallic(TextureHandle metallic);

		void UseMetallicTexture(bool use_metallic);

		TextureHandle GetAmbientOcclusion() { return m_ao; }
		void SetAmbientOcclusion(TextureHandle ao);

		void UseAOTexture(bool use_ao);

		DirectX::XMFLOAT3 GetConstantAlbedo()
		{
			return
			{
				DirectX::XMVectorGetX(m_material_data.m_color),
				DirectX::XMVectorGetY(m_material_data.m_color),
				DirectX::XMVectorGetZ(m_material_data.m_color)
			};
		};

		void SetConstantAlbedo(DirectX::XMFLOAT3 color);

		bool UsesConstantAlbedo() { return m_material_data.m_material_flags.m_use_albedo_constant; };
		void SetUseConstantAlbedo(bool use_constant_albedo) { m_material_data.m_material_flags.m_use_albedo_constant = use_constant_albedo; };

		float GetConstantAlpha() { return DirectX::XMVectorGetW(m_material_data.m_color); };
		void SetConstantAlpha(float alpha);

		DirectX::XMFLOAT3 GetConstantMetallic()
		{
			return
			{
				DirectX::XMVectorGetX(m_material_data.m_metallic_roughness),
				DirectX::XMVectorGetY(m_material_data.m_metallic_roughness),
				DirectX::XMVectorGetZ(m_material_data.m_metallic_roughness)
			};
		};

		void SetConstantMetallic(DirectX::XMFLOAT3 metallic);

		bool UsesConstantMetallic() { return m_material_data.m_material_flags.m_use_metallic_constant; };
		void SetUseConstantMetallic(bool use_constant_metallic) { m_material_data.m_material_flags.m_use_metallic_constant = use_constant_metallic; };

		float GetConstantRoughness() { return DirectX::XMVectorGetW(m_material_data.m_metallic_roughness); };
		void SetConstantRoughness(float roughness);

		bool UsesConstantRoughness() { return m_material_data.m_material_flags.m_use_roughness_constant; };
		void SetUseConstantRoughness(bool use_constant_roughness) { m_material_data.m_material_flags.m_use_roughness_constant = use_constant_roughness; };

		bool IsAlphaMasked() { return m_alpha_masked; };
		void SetAlphaMasked(bool alpha_masked);

		bool IsDoubleSided() { return m_double_sided; };
		void SetDoubleSided(bool double_sided);

		TexturePool* const GetTexturePool() { return m_texture_pool; }

		ConstantBufferHandle* GetConstantBufferHandle() { return m_constant_buffer_handle; };
		void SetConstantBufferHandle(ConstantBufferHandle* handle) { m_constant_buffer_handle = handle; };

		void UpdateConstantBuffer();

		struct MaterialFlags
		{
			unsigned m_has_albedo_texture : 1;
			unsigned m_has_albedo_constant : 1;
			unsigned m_use_albedo_constant : 1;
			unsigned m_has_normal_texture : 1;
			unsigned m_has_roughness_texture : 1;
			unsigned m_has_roughness_constant : 1;
			unsigned m_use_roughness_constant : 1;
			unsigned m_has_metallic_texture : 1;
			unsigned m_has_metallic_constant : 1;
			unsigned m_use_metallic_constant : 1;
			unsigned m_has_ao_texture : 1;
			unsigned m_has_alpha_mask : 1;
			unsigned m_has_constant_alpha : 1;
			unsigned m_is_double_sided : 1;
			unsigned m_placeholder : 18;

		};

		struct MaterialData
		{
			DirectX::XMVECTOR m_color;
			DirectX::XMVECTOR m_metallic_roughness;
			MaterialFlags m_material_flags;
			std::uint32_t padding[3];
		};

		MaterialData GetMaterialData() { return m_material_data; };

	protected:
		TextureHandle m_albedo;
		TextureHandle m_normal;
		TextureHandle m_rougness;
		TextureHandle m_metallic;
		TextureHandle m_ao;

		bool m_alpha_masked = false;
		bool m_double_sided = false;

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
		[[nodiscard]] MaterialHandle Create();
		[[nodiscard]] MaterialHandle Create(TextureHandle& albedo,
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

	protected:
		std::shared_ptr<ConstantBufferPool> m_constant_buffer_pool;

		std::unordered_map<uint64_t, Material*> m_materials;

		IDFactory m_id_factory;
	};

} /* wr */
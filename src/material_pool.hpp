#pragma once

#include <string_view>
#include <unordered_map>
#include <vector>
#include <optional>
#include <stdint.h>
#include <d3d12.h>

#include "structs.hpp"
#include "util/defines.hpp"
#include "id_factory.hpp"

struct aiMaterial;

namespace wr
{
	class Material
	{
	public:

		Material();
		Material(TextureHandle albedo, 
				 TextureHandle normal, 
				 TextureHandle rough_met, 
				 TextureHandle ao, 
				 bool alpha_masked = false, 
				 bool double_sided = false);

		Material(const Material& rhs) = default;

		~Material();

		TextureHandle& Albedo() { return m_albedo; }
		void SetAlbedo(TextureHandle albedo);

		TextureHandle& Normal() { return m_normal; }
		void SetNormal(TextureHandle normal);

		TextureHandle& RoughnessMetallic() { return m_rough_metallic; }
		void SetRoughMetallic(TextureHandle rough_met);

		TextureHandle& AmbientOcclusion() { return m_rough_metallic; }
		void SetAmbientOcclusion(TextureHandle ao);

		bool IsAlphaMasked() { return m_alpha_masked; }
		void SetAlphaMasked(bool alpha_masked) { m_alpha_masked = alpha_masked; }

		bool IsDoubleSided() { return m_double_sided; }
		void SetDoubleSided(bool double_sided) { m_double_sided = double_sided; }

		TexturePool* const GetTexturePool() { return m_texture_pool; }

	protected:
		TextureHandle m_albedo;
		TextureHandle m_normal;
		TextureHandle m_rough_metallic;
		TextureHandle m_ao;

		bool m_alpha_masked = false;
		bool m_double_sided = false;

		TexturePool* m_texture_pool = nullptr;
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
											TextureHandle& rough_met,
											TextureHandle& ao, 
											bool is_alpha_masked = false, 
											bool is_double_sided = false);

		[[nodiscard]] MaterialHandle* Load(aiMaterial* material);

		virtual void Evict() {}
		virtual void MakeResident() {}

		virtual Material* GetMaterial(uint64_t material_id);

	protected:

		std::unordered_map<uint64_t, Material*> m_materials;

		IDFactory m_id_factory;
	};

} /* wr */
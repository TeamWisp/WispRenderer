#pragma once
#include <stdint.h>

namespace wr
{

	struct Texture { };

	class TexturePool;

	struct TextureHandle
	{
		TexturePool* m_pool;
		uint64_t m_id;
	};

	class MaterialPool;

	struct MaterialHandle
	{
		MaterialPool* m_pool;
		uint64_t m_id;

		TextureHandle GetAlbedo() { return m_pool->GetMaterial(m_id)->GetAlbedo(); };
		void SetAlbedo(TextureHandle albedo) { m_pool->GetMaterial(m_id)->SetAlbedo(albedo); };

		TextureHandle GetNormal() { return m_pool->GetMaterial(m_id)->GetNormal(); };
		void SetNormal(TextureHandle normal) { m_pool->GetMaterial(m_id)->SetNormal(normal); };

		TextureHandle GetRoughness() { return m_pool->GetMaterial(m_id)->GetRoughness(); };
		void SetRoughness(TextureHandle roughness) { m_pool->GetMaterial(m_id)->SetRoughness(roughness); };

		TextureHandle GetMetallic() { return m_pool->GetMaterial(m_id)->GetMetallic(); };
		void SetMetallic(TextureHandle metallic) { m_pool->GetMaterial(m_id)->SetMetallic(metallic); };

		TextureHandle GetAmbientOcclusion() { return m_pool->GetMaterial(m_id)->GetAmbientOcclusion(); };
		void SetAmbientOcclusion(TextureHandle ao) { m_pool->GetMaterial(m_id)->SetAmbientOcclusion(ao); };
	};

}
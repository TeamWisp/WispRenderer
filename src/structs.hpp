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

	struct Material
	{
		TextureHandle m_albedo;
		TextureHandle m_normal;
		TextureHandle m_rough_metallic;
		TextureHandle m_ao;

		bool m_alpha_masked = false;
		bool m_double_sided = false;
	};

	class MaterialPool;

	struct MaterialHandle
	{
		MaterialPool* m_pool;
		uint64_t m_id;
	};

}
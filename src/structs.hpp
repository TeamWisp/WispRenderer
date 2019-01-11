#pragma once
#include <stdint.h>

namespace wr
{

	struct Texture { };

	struct ReadbackBuffer { };

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
	};

}
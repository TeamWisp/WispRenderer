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

	struct CPUTexture
	{
		float* m_data = nullptr;
		std::uint64_t m_size = 0;
	};

	class MaterialPool;

	struct MaterialHandle
	{
		MaterialPool* m_pool;
		uint64_t m_id;
	};

}
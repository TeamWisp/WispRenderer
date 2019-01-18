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
		unsigned int m_buffer_width;
		unsigned int m_buffer_height;
		unsigned int m_bytes_per_pixel;
	};

	class MaterialPool;

	struct MaterialHandle
	{
		MaterialPool* m_pool;
		uint64_t m_id;
	};

}
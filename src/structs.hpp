#pragma once
#include <stdint.h>

namespace wr
{

	struct Texture { };

	struct ReadbackBuffer
	{
		// Holds the buffer data of the most recent buffer update
		float* m_data = nullptr;
	};

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
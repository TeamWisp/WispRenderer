#pragma once
#include <stdint.h>
#include <cstdint>

namespace wr
{

	struct Texture { };

	struct ReadbackBuffer { };

	class TexturePool;

	struct TextureHandle
	{
		TexturePool* m_pool = nullptr;
		std::uint32_t m_id = 0;
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
		std::uint32_t m_id;

		friend bool operator ==(MaterialHandle const & lhs, MaterialHandle const & rhs)
		{
			return lhs.m_pool == rhs.m_pool && lhs.m_id == rhs.m_id;
		}

		friend bool operator!=(MaterialHandle const& lhs, MaterialHandle const& rhs)
		{
			return lhs.m_pool != rhs.m_pool || lhs.m_id != rhs.m_id;
		}
	};

}
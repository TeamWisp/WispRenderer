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

		bool operator==(const MaterialHandle& second)
		{
			return (this->m_pool == second.m_pool && this->m_id == second.m_id);
		}
		
		bool operator!=(const MaterialHandle& second)
		{
			return (this->m_pool != second.m_pool || this->m_id != second.m_id);
		}
	};

}
#include "resource_pool_texture.hpp"
#include "util/log.hpp"

namespace wr
{

	TexturePool::TexturePool(std::size_t size_in_mb)
		: m_size_in_mb(size_in_mb)
		, m_is_staged(false)
		, m_count(0)
	{
	}

	//! Loads a texture
	TextureHandle* TexturePool::Load(std::string_view path, TextureType type, bool srgb)
	{
		std::size_t length = path.length();
		std::string_view extension = path.substr(length - 3, 3);

		TextureHandle* texture = nullptr;

		if (extension == "png")
		{
			texture = LoadPNG(path, srgb);
		}
		else if (extension == "dds")
		{
			texture = LoadDDS(path, srgb);
		}
		else if (extension == "hdr")
		{
			texture = LoadHDR(path, srgb);
		}
		else
		{
			LOGC("Texture {} not loaded. Format not supported.", path);
		}

		LOG("[TEXTURE LOADED] {}", path);

		m_textures.push_back(texture);
		m_count++;

		return texture;
	}

}
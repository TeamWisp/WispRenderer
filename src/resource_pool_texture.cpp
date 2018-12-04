#include "resource_pool_texture.hpp"
#include "util/log.hpp"

namespace wr
{

	TexturePool::TexturePool(std::size_t size_in_mb)
		: m_size_in_mb(size_in_mb)
	{
	}

	//! Loads a texture
	TextureHandle TexturePool::Load(std::string_view path, bool srgb)
	{
		std::size_t length = path.length();
		std::string_view extension = path.substr(length - 3, 3);

		Texture* texture;

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
		
		uint64_t texture_id = m_id_factory.GetUnusedID();

		TextureHandle texture_handle;
		texture_handle.m_pool = this;
		texture_handle.m_id = texture_id;

		m_unstaged_textures.insert(std::make_pair(texture_id, texture));

		return texture_handle;
	}

}
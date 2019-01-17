#include "resource_pool_texture.hpp"
#include "util/log.hpp"
#include "util/strings.hpp"

namespace wr
{

	TexturePool::TexturePool(std::size_t size_in_bytes)
		: m_size_in_bytes(size_in_bytes)
	{
	}

	//! Loads a texture
	TextureHandle TexturePool::Load(std::string_view path, bool srgb, bool generate_mips)
	{
		std::optional<std::string_view> extension = util::GetFileExtension(path);

		if (std::string_view ext_int = extension.value(); extension.has_value())
		{
			Texture* texture;

			if (ext_int.find("png") != std::string_view::npos)
			{
				texture = LoadPNG(path, srgb, generate_mips);
			}
			else if (ext_int.find("dds") != std::string_view::npos)
			{
				texture = LoadDDS(path, srgb, generate_mips);
			}
			else if (ext_int.find("hdr") != std::string_view::npos)
			{
				texture = LoadHDR(path, srgb, generate_mips);
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
		else
		{
			LOGC("Invalid path used as argument. Path: {}", path);
		}
	}
}
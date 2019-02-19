#include "resource_pool_texture.hpp"
#include "util/log.hpp"
#include "util/strings.hpp"

namespace wr
{

	TexturePool::TexturePool()
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

	TextureHandle TexturePool::GetDefaultAlbedo()
	{
		return m_default_albedo;
	}

	TextureHandle TexturePool::GetDefaultNormal()
	{
		return m_default_normal;
	}

	TextureHandle TexturePool::GetDefaultRoughness()
	{
		return m_default_roughness;
	}

	TextureHandle TexturePool::GetDefaultMetalic()
	{
		return m_default_metalic;
	}

	TextureHandle TexturePool::GetDefaultAO()
	{
		return m_default_ao;
	}
}
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
			else if (ext_int.find("jpg") != std::string_view::npos)
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

	TextureHandle TexturePool::LoadFromMemory(char * data, int width, int height, TextureType type, bool srgb, bool generate_mips)
	{
		Texture* texture;
		switch (type) {
		case TextureType::DDS:
			texture = LoadDDSFromMemory(data, width, srgb, generate_mips);
			break;
		case TextureType::PNG:
			texture = LoadPNGFromMemory(data, width, srgb, generate_mips);
			break;
		case TextureType::HDR:
			texture = LoadHDRFromMemory(data, width, srgb, generate_mips);
			break;
		case TextureType::RAW:
			texture = LoadRawFromMemory(data, width, height, srgb, generate_mips);
		default:
			LOGC("Texture {} not loaded. Format not supported.");
		}

		uint64_t texture_id = m_id_factory.GetUnusedID();

		TextureHandle texture_handle;
		texture_handle.m_pool = this;
		texture_handle.m_id = texture_id;

		m_unstaged_textures.insert(std::make_pair(texture_id, texture));

		return texture_handle;
	}

	TextureHandle TexturePool::LoadFromMemory(char * data, int width, int height, std::string texture_extension, bool srgb, bool generate_mips)
	{
		std::for_each(texture_extension.begin(), texture_extension.end(), [](char & c) {
			c = ::tolower(c);
		});

		TextureType type;
		if (texture_extension.size() == 0 || height > 1)
		{
			type = TextureType::RAW;
		}
		else if (texture_extension.compare("png") == 0)
		{
			type = TextureType::PNG;
		}
		else if (texture_extension.compare("dds") == 0)
		{
			type = TextureType::DDS;
		}
		else if (texture_extension.compare("hdr") == 0)
		{
			type = TextureType::HDR;
		}
		else
		{
			type = TextureType::PNG;
		}

		return LoadFromMemory(data, width, height, type, srgb, generate_mips);
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
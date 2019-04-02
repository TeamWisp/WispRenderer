#include "resource_pool_texture.hpp"
#include "util/log.hpp"
#include "util/strings.hpp"

namespace wr
{

	TexturePool::TexturePool()
	{
	}

	TextureHandle TexturePool::LoadFromCompressedMemory(char* data, size_t width, size_t height, std::string& texture_extension, bool srgb, bool generate_mips)
	{
		std::for_each(texture_extension.begin(), texture_extension.end(), [](char& c) {
			c = ::tolower(c);
		});

		TextureType type;

		if (texture_extension.compare("png") == 0
			|| texture_extension.compare("jpg") == 0
			|| texture_extension.compare("jpeg") == 0
			|| texture_extension.compare("bmp") == 0)
		{
			type = TextureType::WIC;
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
			LOGC("[ERROR]: Texture format not supported.");
		}

		return LoadFromCompressedMemory(data, width, height, type, srgb, generate_mips);
	}

	TextureHandle TexturePool::GetDefaultAlbedo() const
	{
		return m_default_albedo;
	}

	TextureHandle TexturePool::GetDefaultNormal() const
	{
		return m_default_normal;
	}

	TextureHandle TexturePool::GetDefaultRoughness() const
	{
		return m_default_roughness;
	}

	TextureHandle TexturePool::GetDefaultMetalic() const
	{
		return m_default_metalic;
	}

	TextureHandle TexturePool::GetDefaultAO() const
	{
		return m_default_ao;
	}
}
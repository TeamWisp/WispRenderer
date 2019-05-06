#pragma once

#include <string_view>
#include <vector>
#include <unordered_map>
#include <optional>
#include <d3d12.h>

#include "structs.hpp"
#include "util/defines.hpp"
#include "util/strings.hpp"
#include "platform_independend_structs.hpp"
#include "id_factory.hpp"

namespace wr
{
	enum class TextureType
	{
		WIC,
		DDS,
		HDR
	};

	class TexturePool
	{
	public:
		explicit TexturePool();
		virtual ~TexturePool() = default;

		TexturePool(TexturePool const &) = delete;
		TexturePool& operator=(TexturePool const &) = delete;
		TexturePool(TexturePool&&) = delete;
		TexturePool& operator=(TexturePool&&) = delete;

		[[nodiscard]] virtual TextureHandle LoadFromFile(std::string_view path, bool srgb, bool generate_mips) = 0;
		[[nodiscard]] virtual TextureHandle LoadFromCompressedMemory(unsigned char* data, size_t width, size_t height, const std::string& texture_extension, bool srgb, bool generate_mips);
		[[nodiscard]] virtual TextureHandle LoadFromCompressedMemory(unsigned char* data, size_t width, size_t height, TextureType type, bool srgb, bool generate_mips) = 0;
		[[nodiscard]] virtual TextureHandle LoadFromRawMemory(unsigned char* data, size_t width, size_t height, bool srgb, bool generate_mips) = 0;
		[[nodiscard]] virtual TextureHandle CreateCubemap(std::string_view name, uint32_t width, uint32_t height, uint32_t mip_levels, Format format, bool allow_render_dest) = 0;
		[[nodiscard]] virtual TextureHandle CreateTexture(std::string_view name, uint32_t width, uint32_t height, uint32_t mip_levels, Format format, bool allow_render_dest) = 0;
		virtual void Unload(TextureHandle& handle) = 0;

		virtual void Evict() = 0;
		virtual void MakeResident() = 0;
		virtual void Stage(CommandList* cmd_list) = 0;
		virtual void PostStageClear() = 0;
		virtual void ReleaseTemporaryResources() = 0;

		TextureHandle GetDefaultAlbedo() const;
		TextureHandle GetDefaultNormal() const;
		TextureHandle GetDefaultRoughness() const;
		TextureHandle GetDefaultMetalic() const;
		TextureHandle GetDefaultAO() const;

		virtual Texture* GetTextureResource(TextureHandle handle) = 0;

	protected:

		std::size_t m_loaded_textures = 0;

		TextureHandle m_default_albedo;
		TextureHandle m_default_normal;
		TextureHandle m_default_roughness;
		TextureHandle m_default_metalic;
		TextureHandle m_default_ao;

		IDFactory m_id_factory;

#ifdef _DEBUG
		std::string m_name;
#endif
	};


}

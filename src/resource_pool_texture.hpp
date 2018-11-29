#pragma once

#include <string_view>
#include <vector>
#include <optional>
#include <d3d12.h>

#include "util/defines.hpp"
#include "platform_independend_structs.hpp"

namespace wr
{
	struct TextureHandle {};

	enum class TextureType
	{
		PNG,
		DDS,
		HDR
	};

	class TexturePool
	{
	public:
		explicit TexturePool(std::size_t size_in_mb);
		virtual ~TexturePool() = default;

		TexturePool(TexturePool const &) = delete;
		TexturePool& operator=(TexturePool const &) = delete;
		TexturePool(TexturePool&&) = delete;
		TexturePool& operator=(TexturePool&&) = delete;

		[[nodiscard]] TextureHandle* Load(std::string_view path, bool srgb);

		virtual void Evict() = 0;
		virtual void MakeResident() = 0;
		virtual void Stage(CommandList* cmd_list) = 0;
		virtual void PostStageClear() = 0;

	protected:

		virtual TextureHandle* LoadPNG(std::string_view path, bool srgb) = 0;
		virtual TextureHandle* LoadDDS(std::string_view path, bool srgb) = 0;
		virtual TextureHandle* LoadHDR(std::string_view path, bool srgb) = 0;

		std::vector<TextureHandle*> m_textures;
		size_t m_count;

		std::size_t m_size_in_mb;

		bool m_is_staged;
	};


}

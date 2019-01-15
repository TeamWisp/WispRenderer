#pragma once

#include <string_view>
#include <vector>
#include <unordered_map>
#include <optional>
#include <d3d12.h>

#include "structs.hpp"
#include "util/defines.hpp"
#include "platform_independend_structs.hpp"
#include "id_factory.hpp"

namespace wr
{
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

		[[nodiscard]] TextureHandle Load(std::string_view path, bool srgb, bool generate_mips);
		virtual void Unload(uint64_t texture_id) = 0;

		virtual void Evict() = 0;
		virtual void MakeResident() = 0;
		virtual void Stage(CommandList* cmd_list) = 0;
		virtual void Stage() = 0;
		virtual void WaitForStaging() = 0;
		virtual void PostStageClear() = 0;
		virtual void EndOfFrame() = 0;

		virtual Texture* GetTexture(uint64_t texture_id) = 0;

	protected:

		virtual Texture* LoadPNG(std::string_view path, bool srgb, bool generate_mips) = 0;
		virtual Texture* LoadDDS(std::string_view path, bool srgb, bool generate_mips) = 0;
		virtual Texture* LoadHDR(std::string_view path, bool srgb, bool generate_mips) = 0;

		std::unordered_map<uint64_t, Texture*> m_unstaged_textures;
		std::unordered_map<uint64_t, Texture*> m_staged_textures;

		std::size_t m_size_in_mb;
		std::size_t m_loaded_textures = 0;

		IDFactory m_id_factory;
	};


}

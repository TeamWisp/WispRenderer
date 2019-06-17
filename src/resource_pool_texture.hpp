// Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <string_view>
#include <vector>
#include <unordered_map>
#include <optional>
#include <d3d12.h>
#include <mutex>

#include "structs.hpp"
#include "util/defines.hpp"
#include "util/strings.hpp"
#include "platform_independend_structs.hpp"
#include "id_factory.hpp"

namespace wr
{
	enum class TextureFormat
	{
		WIC,
		DDS,
		HDR,
		RAW
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
		[[nodiscard]] virtual TextureHandle LoadFromMemory(unsigned char* data, size_t width, size_t height, const std::string& texture_extension, bool srgb, bool generate_mips);
		[[nodiscard]] virtual TextureHandle LoadFromMemory(unsigned char* data, size_t width, size_t height, TextureFormat type, bool srgb, bool generate_mips) = 0;
		[[nodiscard]] virtual TextureHandle CreateCubemap(std::string_view name, uint32_t width, uint32_t height, uint32_t mip_levels, Format format, bool allow_render_dest) = 0;
		[[nodiscard]] virtual TextureHandle CreateTexture(std::string_view name, uint32_t width, uint32_t height, uint32_t mip_levels, Format format, bool allow_render_dest) = 0;
		virtual void MarkForUnload(TextureHandle& handle, unsigned int frame_idx) = 0;
		virtual void UnloadTextures(unsigned int frame_idx) = 0;

		virtual void Evict() = 0;
		virtual void MakeResident() = 0;
		virtual void Stage(CommandList* cmd_list) = 0;
		virtual void PostStageClear() = 0;
		virtual void ReleaseTemporaryResources() = 0;

		virtual Texture* GetTextureResource(TextureHandle handle) = 0;

	protected:

		std::size_t m_loaded_textures = 0;
		std::mutex m_mutex;

		IDFactory m_id_factory;

#ifdef _DEBUG
		std::string m_name;
#endif
	};


}

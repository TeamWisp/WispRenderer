/*!
 * Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "resource_pool_texture.hpp"
#include "util/log.hpp"
#include "util/strings.hpp"

namespace wr
{

	TexturePool::TexturePool()
	{
#ifdef _DEBUG
		std::lock_guard<std::mutex> lock(m_mutex);

		static uint16_t pool_count = 0u;
		
		m_name = "TexturePool_" + std::to_string(pool_count);

		pool_count++;
#endif
	}

	TextureHandle TexturePool::LoadFromMemory(unsigned char* data, size_t width, size_t height, const std::string& texture_extension, bool srgb, bool generate_mips)
	{
		std::string new_str = texture_extension;

		std::transform(texture_extension.begin(), texture_extension.end(), new_str.begin(), ::tolower);

		TextureFormat type;

		if (new_str == "png"|| new_str == "jpg"
			|| new_str == "jpeg" || new_str == "bmp")
		{
			type = TextureFormat::WIC;
		}
		else if (new_str.compare("dds") == 0)
		{
			type = TextureFormat::DDS;
		}
		else if (new_str.compare("hdr") == 0)
		{
			type = TextureFormat::HDR;
		}
		else
		{
			LOGC("[ERROR]: Texture format not supported.");
			return {};
		}

		std::lock_guard<std::mutex> lock(m_mutex);
		return LoadFromMemory(data, width, height, type, srgb, generate_mips);
	}
}
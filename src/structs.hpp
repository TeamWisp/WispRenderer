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
#pragma once
#include <stdint.h>
#include <cstdint>

namespace wr
{

	struct Texture { };

	struct ReadbackBuffer { };

	class TexturePool;

	struct TextureHandle
	{
		TexturePool* m_pool = nullptr;
		std::uint32_t m_id = 0;
	};

	struct MaterialUVScales
	{
		float m_albedo_scale = 1.0f;
		float m_normal_scale = 1.0f;
		float m_roughness_scale = 1.0f;
		float m_metallic_scale = 1.0f;
		float m_emissive_scale = 1.0f;
		float m_ao_scale = 1.0f;
	};

	struct CPUTexture
	{
		float* m_data = nullptr;
		unsigned int m_buffer_width;
		unsigned int m_buffer_height;
		unsigned int m_bytes_per_pixel;
	};

	class MaterialPool;

	struct MaterialHandle
	{
		MaterialPool* m_pool;
		std::uint32_t m_id;

		friend bool operator ==(MaterialHandle const & lhs, MaterialHandle const & rhs)
		{
			return lhs.m_pool == rhs.m_pool && lhs.m_id == rhs.m_id;
		}

		friend bool operator!=(MaterialHandle const& lhs, MaterialHandle const& rhs)
		{
			return lhs.m_pool != rhs.m_pool || lhs.m_id != rhs.m_id;
		}
	};

}
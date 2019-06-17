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
#include "../structs.hpp"

#include "d3d12_enums.hpp"
#include "d3d12_settings.hpp"
#include "d3dx12.hpp"

#include "d3d12_descriptors_allocations.hpp"

#include <DirectXTex.h>

namespace wr::d3d12
{
	struct TextureResource : Texture
	{
		std::size_t m_width = 0;
		std::size_t m_height = 0;
		std::size_t m_depth = 0;
		std::size_t m_array_size = 0;
		std::size_t m_mip_levels = 0;

		Format m_format = wr::Format::UNKNOWN;

		ID3D12Resource* m_resource = nullptr;
		ID3D12Resource* m_intermediate = nullptr;
		std::vector<ResourceState> m_subresource_states;
		DescriptorAllocation m_srv_allocation;
		DescriptorAllocation m_uav_allocation;

		std::vector<D3D12_SUBRESOURCE_DATA> m_subresources;

		//This allocation can be either 1 or 6 continous descriptors based on m_is_cubemap
		std::optional<DescriptorAllocation> m_rtv_allocation = std::nullopt;

		bool m_is_staged = false;
		bool m_need_mips = false;
		bool m_is_cubemap = false;
		bool m_allow_render_dest = false;
	};
}
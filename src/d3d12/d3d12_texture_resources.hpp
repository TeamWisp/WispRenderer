#pragma once
#include "../structs.hpp"

#include "d3d12_enums.hpp"
#include "d3d12_settings.hpp"
#include "d3dx12.hpp"

#include "d3d12_descriptors_allocations.hpp"

#include "DirectXTex.h"

namespace wr::d3d12
{
	struct TextureResource : Texture
	{
		std::size_t m_width;
		std::size_t m_height;
		std::size_t m_depth;
		std::size_t m_array_size;
		std::size_t m_mip_levels;

		Format m_format;

		ID3D12Resource* m_resource;
		ID3D12Resource* m_intermediate;
		std::vector<ResourceState> m_subresource_states;
		DescriptorAllocation m_srv_allocation;
		DescriptorAllocation m_uav_allocation;

		std::vector<D3D12_SUBRESOURCE_DATA> m_subresources;
		DirectX::ScratchImage* image;

		//This allocation can be either 1 or 6 continous descriptors based on m_is_cubemap
		std::optional<DescriptorAllocation> m_rtv_allocation = std::nullopt;

		uint8_t* m_allocated_memory;
		size_t m_needed_memory;

		bool m_is_staged = false;
		bool m_need_mips = false;
		bool m_is_cubemap = false;
		bool m_allow_render_dest = false;
	};
}
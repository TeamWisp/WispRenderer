#pragma once
#include "../structs.hpp"

#include "d3d12_enums.hpp"
#include "d3d12_settings.hpp"
#include "d3dx12.hpp"

#include "d3d12_descriptors_allocations.hpp"

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
		ResourceState m_current_state;
		DescriptorAllocation m_desc_allocation;

		uint8_t* m_allocated_memory;

		bool m_is_staged = false;
		bool m_need_mips = false;
		bool m_is_cubemap = false;
	};
}
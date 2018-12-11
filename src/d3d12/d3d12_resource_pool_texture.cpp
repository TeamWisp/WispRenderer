#include "d3d12_resource_pool_texture.hpp"

#include "d3d12_functions.hpp"
#include "d3d12_renderer.hpp"

#include "../renderer.hpp"
#include "d3d12_renderer.hpp"
#include "d3d12_structs.hpp"

#include "DirectXTex.h"

namespace wr
{
	D3D12TexturePool::D3D12TexturePool(D3D12RenderSystem& render_system, std::size_t size_in_mb, std::size_t num_of_textures)
		: TexturePool(size_in_mb)
		, m_render_system(render_system)
	{
		auto device = m_render_system.m_device;

		d3d12::desc::DescriptorHeapDesc desc;
		desc.m_num_descriptors = num_of_textures;
		desc.m_type = DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV;

		m_descriptor_heap = d3d12::CreateDescriptorHeap(device, desc);
		SetName(m_descriptor_heap, L"Texture Pool Descriptor Heap");
		m_descriptor_handle = d3d12::GetCPUHandle(m_descriptor_heap, 0);
	}

	D3D12TexturePool::~D3D12TexturePool()
	{
	}

	void D3D12TexturePool::Evict()
	{
	}

	void D3D12TexturePool::MakeResident()
	{
	}

	void D3D12TexturePool::Stage(CommandList* cmd_list)
	{
		size_t unstaged_number = m_unstaged_textures.size();

		if (unstaged_number > 0)
		{
			d3d12::CommandList* cmdlist = static_cast<d3d12::CommandList*>(cmd_list);

			std::vector<d3d12::TextureResource*> unstaged_textures;

			auto itr = m_unstaged_textures.begin();

			for (itr; itr != m_unstaged_textures.end(); ++itr)
			{
				d3d12::TextureResource* texture = static_cast<d3d12::TextureResource*>(itr->second);

				unstaged_textures.push_back(texture);

				size_t row_pitch, slice_pitch;

				DirectX::ComputePitch(static_cast<DXGI_FORMAT>(texture->m_format), texture->m_width, texture->m_height, row_pitch, slice_pitch);

				D3D12_SUBRESOURCE_DATA subresourceData = {};
				subresourceData.pData = texture->m_allocated_memory;
				subresourceData.RowPitch = row_pitch;
				subresourceData.SlicePitch = slice_pitch;

				UpdateSubresources(cmdlist->m_native, texture->m_resource, texture->m_intermediate, 0, 0, 1, &subresourceData);

				texture->m_is_staged = true;
			}

			d3d12::Transition(cmdlist, unstaged_textures, wr::ResourceState::COPY_DEST, wr::ResourceState::PIXEL_SHADER_RESOURCE);

			MoveStagedTextures();
		}
	}

	void D3D12TexturePool::PostStageClear()
	{
	}

	d3d12::TextureResource * D3D12TexturePool::GetTexture(uint64_t texture_id)
	{
		return nullptr;
	}

	d3d12::TextureResource* D3D12TexturePool::LoadPNG(std::string_view path, bool srgb)
	{
		auto device = m_render_system.m_device;

		DirectX::TexMetadata metadata;
		DirectX::ScratchImage image;

		std::wstring wide_string(path.begin(), path.end());

		HRESULT hr = LoadFromWICFile(wide_string.c_str(),
			DirectX::WIC_FLAGS_NONE, &metadata, image);

		if (FAILED(hr))
		{
			LOGC("ERROR: Texture not loaded correctly.");
		}

		d3d12::desc::TextureDesc desc;

		desc.m_width = metadata.width;
		desc.m_height = metadata.height;
		desc.m_is_cubemap = metadata.IsCubemap();
		desc.m_depth = metadata.depth;
		desc.m_array_size = metadata.arraySize;
		desc.m_mip_levels = metadata.mipLevels;
		desc.m_texture_format = static_cast<wr::Format>(metadata.format);
		desc.m_initial_state = ResourceState::COPY_DEST;

		auto texture = d3d12::CreateTexture(device, &desc, false);

		texture->m_need_mips = (metadata.mipLevels > 1);
		texture->m_allocated_memory = static_cast<uint8_t*>(malloc(image.GetPixelsSize()));

		memcpy(texture->m_allocated_memory, image.GetPixels(), image.GetPixelsSize());

		texture->m_cpu_descriptor_handle = m_descriptor_handle;
		texture->m_resource->SetName(wide_string.c_str());

		d3d12::CreateSRVFromTexture(texture, texture->m_cpu_descriptor_handle, desc.m_texture_format);

		return texture;
	}

	d3d12::TextureResource* D3D12TexturePool::LoadDDS(std::string_view path, bool srgb)
	{
		auto device = m_render_system.m_device;

		DirectX::TexMetadata metadata;
		DirectX::ScratchImage image;

		std::wstring wide_string(path.begin(), path.end());

		HRESULT hr = LoadFromDDSFile(wide_string.c_str(),
			DirectX::DDS_FLAGS_NONE, &metadata, image);

		if (FAILED(hr))
		{
			LOGC("ERROR: Texture not loaded correctly.");
		}

		d3d12::desc::TextureDesc desc;

		desc.m_width = metadata.width;
		desc.m_height = metadata.height;
		desc.m_is_cubemap = metadata.IsCubemap();
		desc.m_depth = metadata.depth;
		desc.m_array_size = metadata.arraySize;
		desc.m_mip_levels = metadata.mipLevels;
		desc.m_texture_format = static_cast<wr::Format>(metadata.format);
		desc.m_initial_state = ResourceState::COPY_DEST;

		auto texture = d3d12::CreateTexture(device, &desc, false);

		texture->m_need_mips = (metadata.mipLevels > 1) ? false : true;
		texture->m_allocated_memory = static_cast<uint8_t*>(malloc(image.GetPixelsSize()));

		memcpy(texture->m_allocated_memory, image.GetPixels(), image.GetPixelsSize());

		texture->m_cpu_descriptor_handle = m_descriptor_handle;
		texture->m_resource->SetName(wide_string.c_str());

		d3d12::CreateSRVFromTexture(texture, texture->m_cpu_descriptor_handle, desc.m_texture_format);

		return texture;
	}

	d3d12::TextureResource* D3D12TexturePool::LoadHDR(std::string_view path, bool srgb)
	{
		auto device = m_render_system.m_device;

		DirectX::TexMetadata metadata;
		DirectX::ScratchImage image;

		std::wstring wide_string(path.begin(), path.end());

		HRESULT hr = LoadFromHDRFile(wide_string.c_str(), &metadata, image);

		if (FAILED(hr))
		{
			LOGC("ERROR: Texture not loaded correctly.");
		}

		d3d12::desc::TextureDesc desc;

		desc.m_width = metadata.width;
		desc.m_height = metadata.height;
		desc.m_is_cubemap = metadata.IsCubemap();
		desc.m_depth = metadata.depth;
		desc.m_array_size = metadata.arraySize;
		desc.m_mip_levels = metadata.mipLevels;
		desc.m_texture_format = static_cast<wr::Format>(metadata.format);
		desc.m_initial_state = ResourceState::COPY_DEST;

		auto texture = d3d12::CreateTexture(device, &desc, false);

		texture->m_need_mips = (metadata.mipLevels > 1) ? false : true;
		texture->m_allocated_memory = static_cast<uint8_t*>(malloc(image.GetPixelsSize()));

		memcpy(texture->m_allocated_memory, image.GetPixels(), image.GetPixelsSize());

		texture->m_cpu_descriptor_handle = m_descriptor_handle;
		texture->m_resource->SetName(wide_string.c_str());

		d3d12::CreateSRVFromTexture(texture, texture->m_cpu_descriptor_handle, desc.m_texture_format);

		return texture;
	}

	void D3D12TexturePool::MoveStagedTextures()
	{
		for (auto itr = m_unstaged_textures.begin(); itr != m_unstaged_textures.end(); ++itr)
		{
			m_staged_textures.insert(std::make_pair(itr->first, itr->second));
		}

		m_unstaged_textures.clear();
	}
}
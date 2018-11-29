#include "d3d12_resource_pool_texture.hpp"

#include "d3d12_functions.hpp"
#include "d3d12_renderer.hpp"

#include "../renderer.hpp"
#include "d3d12_renderer.hpp"
#include "d3d12_structs.hpp"

#include "third_party/DirectXTex/DirectXTex.h"

namespace wr
{
	D3D12TexturePool::D3D12TexturePool(D3D12RenderSystem& render_system, std::size_t size_in_mb, std::size_t num_of_textures)
		: TexturePool(size_in_mb)
		, m_render_system(render_system)
	{
		auto device = m_render_system.m_device;

		d3d12::desc::DescriptorHeapDesc desc;
		desc.m_num_descriptors = size_in_mb / num_of_textures;
		desc.m_type = DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV;

		m_descriptor_heap = d3d12::CreateDescriptorHeap(device, desc);
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
		if (!m_is_staged)
		{
			D3D12CommandList* cmdlist = static_cast<D3D12CommandList*>(cmd_list);

			std::vector<d3d12::Texture*> textures;
			textures.resize(m_count);

			for (uint32_t i = 0; i < m_count; i++)
			{
				d3d12::Texture* native = static_cast<D3D12TextureHandle*>(m_textures[i])->m_native;

				textures[i] = native;

				size_t row_pitch, slice_pitch;

				DirectX::ComputePitch(static_cast<DXGI_FORMAT>(native->m_format), native->m_width, native->m_height, row_pitch, slice_pitch);

				D3D12_SUBRESOURCE_DATA subresourceData = {};
				subresourceData.pData = native->m_allocated_memory;
				subresourceData.RowPitch = row_pitch;
				subresourceData.SlicePitch = slice_pitch;

				UpdateSubresources(cmdlist->m_native, native->m_resource, native->m_intermediate, 0, 0, 1, &subresourceData);
			}

			d3d12::Transition(cmdlist, textures, wr::ResourceState::COPY_DEST, wr::ResourceState::PIXEL_SHADER_RESOURCE);

			m_is_staged = true;
		}
	}

	void D3D12TexturePool::PostStageClear()
	{
	}

	D3D12TextureHandle* D3D12TexturePool::LoadPNG(std::string_view path, bool srgb)
	{
		auto texture = new D3D12TextureHandle();
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

		auto native = d3d12::CreateTexture(device, &desc, false);

		native->m_need_mips = (metadata.mipLevels > 1) ? false : true;
		native->m_allocated_memory = static_cast<uint8_t*>(malloc(image.GetPixelsSize()));

		memcpy(native->m_allocated_memory, image.GetPixels(), image.GetPixelsSize());

		texture->m_native = native;

		native->m_CpuDescriptorHandle = m_descriptor_handle;

		DXGI_FORMAT format = metadata.format;

		d3d12::CreateSRVFromTexture(native, native->m_CpuDescriptorHandle, (Format)format);

		m_textures.push_back(texture);

		return texture;
	}

	D3D12TextureHandle* D3D12TexturePool::LoadDDS(std::string_view path, bool srgb)
	{
		auto texture = new D3D12TextureHandle();
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

		auto native = d3d12::CreateTexture(device, &desc, false);

		native->m_need_mips = (metadata.mipLevels > 1) ? false : true;
		native->m_allocated_memory = static_cast<uint8_t*>(malloc(image.GetPixelsSize()));

		memcpy(native->m_allocated_memory, image.GetPixels(), image.GetPixelsSize());

		texture->m_native = native;

		native->m_CpuDescriptorHandle = m_descriptor_handle;

		DXGI_FORMAT format = metadata.format;

		d3d12::CreateSRVFromTexture(native, native->m_CpuDescriptorHandle, (Format)format);

		m_textures.push_back(texture);

		return texture;
	}

	D3D12TextureHandle* D3D12TexturePool::LoadHDR(std::string_view path, bool srgb)
	{
		auto texture = new D3D12TextureHandle();
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

		auto native = d3d12::CreateTexture(device, &desc, false);

		native->m_need_mips = (metadata.mipLevels > 1) ? false : true;
		native->m_allocated_memory = static_cast<uint8_t*>(malloc(image.GetPixelsSize()));

		memcpy(native->m_allocated_memory, image.GetPixels(), image.GetPixelsSize());

		texture->m_native = native;

		native->m_CpuDescriptorHandle = m_descriptor_handle;

		DXGI_FORMAT format = metadata.format;

		d3d12::CreateSRVFromTexture(native, native->m_CpuDescriptorHandle, (Format)format);

		m_textures.push_back(texture);

		return texture;
	}

	void D3D12TexturePool::LoadTextureResource(CommandList * cmd_list, d3d12::Texture * destination, ID3D12Resource * intermediate)
	{



	}


}
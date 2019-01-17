#include "d3d12_resource_pool_texture.hpp"

#include "d3d12_functions.hpp"
#include "d3d12_defines.hpp"
#include "d3d12_renderer.hpp"

#include "../renderer.hpp"
#include "d3d12_renderer.hpp"
#include "d3d12_structs.hpp"
#include "../d3d12/d3d12_pipeline_registry.hpp"
#include "d3d12_descriptors_allocations.hpp"

#include "DirectXTex.h"

namespace wr
{
	D3D12TexturePool::D3D12TexturePool(D3D12RenderSystem& render_system, std::size_t size_in_bytes, std::size_t num_of_textures)
		: TexturePool(SizeAlign(size_in_bytes, 65536))
		, m_render_system(render_system)
	{
		auto device = m_render_system.m_device;

		//Staging heap
		m_texture_heap_allocator = new DescriptorAllocator(render_system, DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV);

		//Mipmapping heap creation
		d3d12::desc::DescriptorHeapDesc mipmap_heap_desc;
		mipmap_heap_desc.m_num_descriptors = num_of_textures * 12 * 2;
		mipmap_heap_desc.m_type = DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV;
		mipmap_heap_desc.m_shader_visible = true;

		m_mipmapping_heap = d3d12::CreateDescriptorHeap(device, mipmap_heap_desc);
		SetName(m_mipmapping_heap, L"MipMapping Descriptor Heap");
		m_mipmapping_cpu_handle = d3d12::GetCPUHandle(m_mipmapping_heap, 0);
		m_mipmapping_gpu_handle = d3d12::GetGPUHandle(m_mipmapping_heap, 0);

		m_post_stage_clear_textures.resize(d3d12::settings::num_back_buffers);

	}

	D3D12TexturePool::~D3D12TexturePool()
	{
		d3d12::Destroy(m_mipmapping_heap);

		delete m_texture_heap_allocator;
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

			int frame = m_render_system.GetFrameIdx();

			std::vector<d3d12::TextureResource*> unstaged_textures;

			auto itr = m_unstaged_textures.begin();

			for (itr; itr != m_unstaged_textures.end(); ++itr)
			{
				d3d12::TextureResource* texture = static_cast<d3d12::TextureResource*>(itr->second);

				unstaged_textures.push_back(texture);
				m_post_stage_clear_textures[frame].push_back(texture);

				size_t row_pitch, slice_pitch;

				DirectX::ComputePitch(static_cast<DXGI_FORMAT>(texture->m_format), texture->m_width, texture->m_height, row_pitch, slice_pitch);

				D3D12_SUBRESOURCE_DATA subresourceData = {};
				subresourceData.pData = texture->m_allocated_memory;
				subresourceData.RowPitch = row_pitch;
				subresourceData.SlicePitch = slice_pitch;

				UpdateSubresources(cmdlist->m_native, texture->m_resource, texture->m_intermediate, 0, 0, 1, &subresourceData);

				free(texture->m_allocated_memory);

				texture->m_is_staged = true;
			}

			d3d12::Transition(cmdlist, unstaged_textures, wr::ResourceState::COPY_DEST, wr::ResourceState::PIXEL_SHADER_RESOURCE);

			GenerateMips(unstaged_textures, cmd_list);

			MoveStagedTextures();
		}
	}

	void D3D12TexturePool::PostStageClear()
	{
		int frame = m_render_system.GetFrameIdx();
		for (int i = 0; i < m_post_stage_clear_textures[frame].size(); ++i)
		{
			m_post_stage_clear_textures[frame][i]->m_intermediate->Release();
		}

		m_post_stage_clear_textures[frame].clear();
	}

	void D3D12TexturePool::EndOfFrame()
	{
		m_texture_heap_allocator->ReleaseStaleDescriptors();
	}

	d3d12::TextureResource* D3D12TexturePool::GetTexture(uint64_t texture_id)
	{
		return static_cast<d3d12::TextureResource*>(m_staged_textures.at(texture_id));
	}

	void D3D12TexturePool::Unload(uint64_t texture_id)
	{
		d3d12::TextureResource* texture = static_cast<d3d12::TextureResource*>(m_staged_textures.at(texture_id));
		m_staged_textures.erase(texture_id);

		delete[] texture->m_allocated_memory;
		delete texture;
	}

	d3d12::TextureResource* D3D12TexturePool::LoadPNG(std::string_view path, bool srgb, bool generate_mips)
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

		uint32_t mip_lvls;

		if (generate_mips)
		{
			mip_lvls = static_cast<uint32_t>(std::floor(std::log2(std::max(metadata.width, metadata.height)))) + 1;
		}
		else
		{
			mip_lvls = metadata.mipLevels;
		}

		d3d12::desc::TextureDesc desc;

		desc.m_width = metadata.width;
		desc.m_height = metadata.height;
		desc.m_is_cubemap = metadata.IsCubemap();
		desc.m_depth = metadata.depth;
		desc.m_array_size = metadata.arraySize;
		desc.m_mip_levels = mip_lvls;
		desc.m_texture_format = static_cast<wr::Format>(metadata.format);
		desc.m_initial_state = ResourceState::COPY_DEST;

		auto texture = d3d12::CreateTexture(device, &desc, generate_mips);

		texture->m_allocated_memory = static_cast<uint8_t*>(malloc(image.GetPixelsSize()));

		memcpy(texture->m_allocated_memory, image.GetPixels(), image.GetPixelsSize());

		DescriptorAllocation alloc = m_texture_heap_allocator->Allocate();

		if (alloc.IsNull())
		{
			LOGC("Couldn't allocate descriptor for the texture resource");
		}

		texture->m_need_mips = generate_mips;
		texture->m_desc_allocation = std::move(alloc);
		texture->m_resource->SetName(wide_string.c_str());

		d3d12::CreateSRVFromTexture(texture);

		m_loaded_textures++;

		return texture;
	}

	d3d12::TextureResource* D3D12TexturePool::LoadDDS(std::string_view path, bool srgb, bool generate_mips)
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

		uint32_t mip_lvls;

		if (generate_mips)
		{
			mip_lvls = static_cast<uint32_t>(std::floor(std::log2(std::max(metadata.width, metadata.height)))) + 1;
		}
		else
		{
			mip_lvls = metadata.mipLevels;
		}

		d3d12::desc::TextureDesc desc;

		desc.m_width = metadata.width;
		desc.m_height = metadata.height;
		desc.m_is_cubemap = metadata.IsCubemap();
		desc.m_depth = metadata.depth;
		desc.m_array_size = metadata.arraySize;
		desc.m_mip_levels = mip_lvls;
		desc.m_texture_format = static_cast<wr::Format>(metadata.format);
		desc.m_initial_state = ResourceState::COPY_DEST;

		auto texture = d3d12::CreateTexture(device, &desc, generate_mips);

		texture->m_allocated_memory = static_cast<uint8_t*>(malloc(image.GetPixelsSize()));

		memcpy(texture->m_allocated_memory, image.GetPixels(), image.GetPixelsSize());

		DescriptorAllocation alloc = m_texture_heap_allocator->Allocate();

		if (alloc.IsNull())
		{
			LOGC("Couldn't allocate descriptor for the texture resource");
		}

		texture->m_need_mips = generate_mips;
		texture->m_desc_allocation = std::move(alloc);
		texture->m_resource->SetName(wide_string.c_str());

		d3d12::CreateSRVFromTexture(texture);

		m_loaded_textures++;

		return texture;
	}

	d3d12::TextureResource* D3D12TexturePool::LoadHDR(std::string_view path, bool srgb, bool generate_mips)
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

		uint32_t mip_lvls;

		if (generate_mips)
		{
			mip_lvls = static_cast<uint32_t>(std::floor(std::log2(std::max(metadata.width, metadata.height)))) + 1;
		}
		else
		{
			mip_lvls = metadata.mipLevels;
		}

		d3d12::desc::TextureDesc desc;

		desc.m_width = metadata.width;
		desc.m_height = metadata.height;
		desc.m_is_cubemap = metadata.IsCubemap();
		desc.m_depth = metadata.depth;
		desc.m_array_size = metadata.arraySize;
		desc.m_mip_levels = mip_lvls;
		desc.m_texture_format = static_cast<wr::Format>(metadata.format);
		desc.m_initial_state = ResourceState::COPY_DEST;

		auto texture = d3d12::CreateTexture(device, &desc, generate_mips);

		texture->m_allocated_memory = static_cast<uint8_t*>(malloc(image.GetPixelsSize()));

		memcpy(texture->m_allocated_memory, image.GetPixels(), image.GetPixelsSize());

		DescriptorAllocation alloc = m_texture_heap_allocator->Allocate();

		if (alloc.IsNull())
		{
			LOGC("Couldn't allocate descriptor for the texture resource");
		}

		texture->m_need_mips = generate_mips;
		texture->m_desc_allocation = std::move(alloc);
		texture->m_resource->SetName(wide_string.c_str());

		d3d12::CreateSRVFromTexture(texture);

		m_loaded_textures++;

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

	void D3D12TexturePool::GenerateMips(std::vector<d3d12::TextureResource*>& const textures, CommandList * cmd_list)
	{
		wr::d3d12::CommandList* d3d12_cmd_list = static_cast<wr::d3d12::CommandList*>(cmd_list);

		D3D12Pipeline* pipeline = static_cast<D3D12Pipeline*>(PipelineRegistry::Get().Find(pipelines::mip_mapping));

		auto device = m_render_system.m_device;

		//Prepare the shader resource view description for the source texture
		D3D12_SHADER_RESOURCE_VIEW_DESC srcTextureSRVDesc = {};
		srcTextureSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srcTextureSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

		//Prepare the unordered access view description for the destination texture
		D3D12_UNORDERED_ACCESS_VIEW_DESC destTextureUAVDesc = {};
		destTextureUAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

		d3d12::BindDescriptorHeap(d3d12_cmd_list, m_mipmapping_heap, m_mipmapping_heap->m_create_info.m_type, 0);
		d3d12::BindComputePipeline(d3d12_cmd_list, pipeline->m_native);

		m_mipmapping_cpu_handle = d3d12::GetCPUHandle(m_mipmapping_heap, 0);
		m_mipmapping_gpu_handle = d3d12::GetGPUHandle(m_mipmapping_heap, 0);

		d3d12::Transition(d3d12_cmd_list, textures, ResourceState::PIXEL_SHADER_RESOURCE, ResourceState::UNORDERED_ACCESS);

		for (size_t i = 0; i < textures.size(); ++i)
		{
			d3d12::TextureResource* texture = textures[i];

			if (texture->m_need_mips)
			{
				DXGI_FORMAT texture_format = static_cast<DXGI_FORMAT>(texture->m_format);

				for (uint32_t TopMip = 0; TopMip < texture->m_mip_levels - 1; TopMip++)
				{
					uint32_t srcWidth = texture->m_width >> TopMip;
					uint32_t srcHeight = texture->m_height >> TopMip;
					uint32_t dstWidth = srcWidth >> 1;
					uint32_t dstHeight = srcHeight >> 1;

					//Create shader resource view for the source texture in the descriptor heap
					srcTextureSRVDesc.Format = texture_format;
					srcTextureSRVDesc.Texture2D.MipLevels = 1;
					srcTextureSRVDesc.Texture2D.MostDetailedMip = TopMip;
					device->m_native->CreateShaderResourceView(texture->m_resource, &srcTextureSRVDesc, m_mipmapping_cpu_handle.m_native);
					
					d3d12::Offset(m_mipmapping_cpu_handle, 1, m_mipmapping_heap->m_increment_size);

					//Create unordered access view for the destination texture in the descriptor heap
					destTextureUAVDesc.Format = texture_format;
					destTextureUAVDesc.Texture2D.MipSlice = TopMip + 1;
					device->m_native->CreateUnorderedAccessView(texture->m_resource, nullptr, &destTextureUAVDesc, m_mipmapping_cpu_handle.m_native);

					d3d12::Offset(m_mipmapping_cpu_handle, 1, m_mipmapping_heap->m_increment_size);

					auto n_cmd_list = static_cast<d3d12::CommandList*>(cmd_list);

					struct DWParam
					{
						DWParam(FLOAT f) : Float(f) {}
						DWParam(UINT u) : Uint(u) {}

						void operator= (FLOAT f) { Float = f; }
						void operator= (UINT u) { Uint = u; }

						union
						{
							FLOAT Float;
							UINT Uint;
						};
					};

					UINT srcData[] =
					{
						DWParam(1.0f / dstWidth).Uint,
						DWParam(1.0f / dstHeight).Uint
					};

					d3d12::BindCompute32BitConstants(n_cmd_list, srcData, _countof(srcData), 0, 0);

					//Pass the source and destination texture views to the shader via descriptor tables
					d3d12::BindComputeDescriptorTable(n_cmd_list, m_mipmapping_gpu_handle, 1);

					d3d12::Offset(m_mipmapping_gpu_handle, 2, m_mipmapping_heap->m_increment_size);
					
					//Dispatch the compute shader with one thread per 8x8 pixels
					d3d12::Dispatch(n_cmd_list, std::max(dstWidth / 8, 1u), std::max(dstHeight / 8, 1u), 1);

					//Wait for all accesses to the destination texture UAV to be finished before generating the next mipmap, as it will be the source texture for the next mipmap
					d3d12::UAVBarrier(n_cmd_list, texture, 1);
				}

				texture->m_need_mips = false;
			}
		}

		d3d12::Transition(d3d12_cmd_list, textures, ResourceState::UNORDERED_ACCESS, ResourceState::PIXEL_SHADER_RESOURCE);

		//if (texture->m_need_mips)
		//{
		//	if (texture->m_depth > 1)
		//	{
		//		LOGC("GenerateMips only supports 2D textures.");
		//	}

		//	if (CheckUAVCompatibility(texture->m_format))
		//	{
		//		GenerateMips_UAV(texture, cmd_list);
		//	}
		//	else if (CheckBGRFormat(texture->m_format))
		//	{
		//		GenerateMips_BGR(texture, cmd_list);
		//	}
		//	else if (CheckSRGBFormat(texture->m_format))
		//	{
		//		GenerateMips_SRGB(texture, cmd_list);
		//	}
		//	else
		//	{
		//		LOGC("Texture format is not supported by GenerateMips.");
		//	}
		//}

	}

	void D3D12TexturePool::GenerateMips_UAV(std::vector<d3d12::TextureResource*>& const textures, CommandList* cmd_list)
	{
	}

	void D3D12TexturePool::GenerateMips_BGR(std::vector<d3d12::TextureResource*>& const textures, CommandList* cmd_list)
	{

	}

	void D3D12TexturePool::GenerateMips_SRGB(std::vector<d3d12::TextureResource*>& const textures, CommandList* cmd_list)
	{
	}

	bool D3D12TexturePool::CheckUAVCompatibility(Format format)
	{
		switch (format)
		{
		case Format::R32G32B32A32_FLOAT:
		case Format::R32G32B32A32_UINT:
		case Format::R32G32B32A32_SINT:
		case Format::R16G16B16A16_FLOAT:
		//case Format::R16G16B16A16_UNORM:
		case Format::R16G16B16A16_UINT:
		case Format::R16G16B16A16_SINT:
		case Format::R8G8B8A8_UNORM:
		case Format::R8G8B8A8_UINT:
		case Format::R8G8B8A8_SINT:
		case Format::R32_FLOAT:
		case Format::R32_UINT:
		case Format::R32_SINT:
		case Format::R16_FLOAT:
		case Format::R16_UINT:
		case Format::R16_SINT:
		case Format::R8_UNORM:
		case Format::R8_UINT:
		case Format::R8_SINT:
			return true;
		default:
			return false;
		}
	}

	bool D3D12TexturePool::CheckBGRFormat(Format format)
	{
		switch (format)
		{
		case Format::B8G8R8A8_UNORM:
		case Format::B8G8R8X8_UNORM:
		case Format::B8G8R8A8_UNORM_SRGB:
		case Format::B8G8R8X8_UNORM_SRGB:
			return true;
		default:
			return false;
		}
	}

	bool D3D12TexturePool::CheckSRGBFormat(Format format)
	{
		switch (format)
		{
		case Format::R8G8B8A8_UNORM_SRGB:
		case Format::B8G8R8A8_UNORM_SRGB:
		case Format::B8G8R8X8_UNORM_SRGB:
			return true;
		default:
			return false;
		}
	}
}
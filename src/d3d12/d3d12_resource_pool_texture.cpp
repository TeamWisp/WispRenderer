#include "d3d12_resource_pool_texture.hpp"

#include "d3d12_functions.hpp"
#include "d3d12_renderer.hpp"

#include "../renderer.hpp"
#include "../settings.hpp"
#include "d3d12_renderer.hpp"
#include "d3d12_structs.hpp"
#include "../d3d12/d3d12_pipeline_registry.hpp"
#include "d3d12_descriptors_allocations.hpp"

#include "DirectXTex.h"

namespace wr
{
	D3D12TexturePool::D3D12TexturePool(D3D12RenderSystem& render_system, std::size_t size_in_mb, std::size_t num_of_textures)
		: TexturePool(size_in_mb)
		, m_render_system(render_system)
	{
		auto device = m_render_system.m_device;

		// Append size and num values for the default textures.
		num_of_textures += settings::default_textures_count;
		size_in_mb += settings::default_textures_size_in_mb;

		//Staging heap

		for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		{
			m_allocators[i] = new DescriptorAllocator(render_system, static_cast<DescriptorHeapType>(i));
		}

		m_mipmapping_allocator = new DescriptorAllocator(render_system, DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV);

		// Load the default textures
		m_default_albedo = Load(settings::default_albedo_path, false, false);
		m_default_normal = Load(settings::default_normal_path, false, false);
		m_default_roughness = Load(settings::default_roughness_path, false, false);
		m_default_metalic = Load(settings::default_metalic_path, false, false);
		m_default_ao = Load(settings::default_ao_path, false, false);
	}

	D3D12TexturePool::~D3D12TexturePool()
	{
		delete m_mipmapping_allocator;

		for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		{
			delete m_allocators[i];
		}
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
			std::vector<d3d12::TextureResource*> need_mipmapping;

			auto itr = m_unstaged_textures.begin();

			for (itr; itr != m_unstaged_textures.end(); ++itr)
			{
				d3d12::TextureResource* texture = static_cast<d3d12::TextureResource*>(itr->second);

				unstaged_textures.push_back(texture);

				decltype(d3d12::Device::m_native) n_device;
				texture->m_resource->GetDevice(IID_PPV_ARGS(&n_device));

				std::uint32_t num_subresources = texture->m_array_size * texture->m_mip_levels;

				std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints;
				footprints.resize(num_subresources);
				std::vector<UINT> num_rows;
				num_rows.resize(num_subresources);
				std::vector<UINT64> row_sizes;
				row_sizes.resize(num_subresources);
				UINT64 total_size;

				D3D12_RESOURCE_DESC desc = texture->m_resource->GetDesc();
				n_device->GetCopyableFootprints(&desc, 0, num_subresources, 0, &footprints[0], &num_rows[0], &row_sizes[0], &total_size);

				std::vector<D3D12_SUBRESOURCE_DATA> subresource_data;
				subresource_data.resize(num_subresources);

				for (uint32_t i = 0; i < num_subresources; ++i)
				{
					D3D12_SUBRESOURCE_FOOTPRINT& footprint = footprints[i].Footprint;

					size_t row_pitch, slice_pitch;

					DirectX::ComputePitch(footprint.Format, footprint.Width, footprint.Height, row_pitch, slice_pitch);

					subresource_data[i].pData = texture->m_allocated_memory + footprints[i].Offset;
					subresource_data[i].RowPitch = row_pitch;
					subresource_data[i].SlicePitch = slice_pitch;
				}

				UpdateSubresources(cmdlist->m_native, texture->m_resource, texture->m_intermediate, 0, num_subresources, total_size, &footprints[0], &num_rows[0], &row_sizes[0], &subresource_data[0]);

				texture->m_is_staged = true;

				if (texture->m_need_mips)
				{
					need_mipmapping.push_back(texture);
				}
			}

			d3d12::Transition(cmdlist, unstaged_textures, wr::ResourceState::COPY_DEST, wr::ResourceState::PIXEL_SHADER_RESOURCE);

			GenerateMips(need_mipmapping, cmd_list);

			MoveStagedTextures();
		}
	}

	void D3D12TexturePool::PostStageClear()
	{
	}

	void D3D12TexturePool::EndOfFrame()
	{
		m_mipmapping_allocator->ReleaseStaleDescriptors();

		for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		{
			m_allocators[i]->ReleaseStaleDescriptors();
		}

		//Release temporary textures
		for (auto* t : m_temporary_textures)
		{
			d3d12::Destroy(t);
		}

		m_temporary_textures.clear();
	}

	d3d12::TextureResource* D3D12TexturePool::GetTexture(uint64_t texture_id)
	{
		return static_cast<d3d12::TextureResource*>(m_staged_textures.at(texture_id));
	}

	TextureHandle D3D12TexturePool::CreateCubemap(std::string_view name, uint32_t width, uint32_t height, uint32_t mip_levels, Format format, bool allow_render_dest)
	{
		auto device = m_render_system.m_device;

		d3d12::desc::TextureDesc desc;

		uint32_t mip_lvl_final = mip_levels;

		// 0 means generate max number of mip levels
		if (mip_lvl_final == 0)
		{
			mip_lvl_final = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
		}

		desc.m_width = width;
		desc.m_height = height;
		desc.m_is_cubemap = true;
		desc.m_depth = 1;
		desc.m_array_size = 6;
		desc.m_mip_levels = mip_lvl_final;
		desc.m_texture_format = format;
		desc.m_initial_state = allow_render_dest ? ResourceState::RENDER_TARGET : ResourceState::COPY_DEST;

		d3d12::TextureResource* texture = d3d12::CreateTexture(device, &desc, true);

		texture->m_allocated_memory = nullptr;
		texture->m_allow_render_dest = allow_render_dest;
		texture->m_is_staged = true;
		texture->m_need_mips = false;

		std::wstring wide_string(name.begin(), name.end());

		d3d12::SetName(texture, wide_string);

		DescriptorAllocation srv_alloc = m_allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Allocate();
		DescriptorAllocation uav_alloc = m_allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Allocate();

		if (srv_alloc.IsNull())
		{
			LOGC("Couldn't allocate descriptor for the texture resource");
		}
		if (uav_alloc.IsNull())
		{
			LOGC("Couldn't allocate descriptor for the texture resource");
		}

		texture->m_srv_allocation = std::move(srv_alloc);
		texture->m_uav_allocation = std::move(uav_alloc);

		d3d12::CreateSRVFromTexture(texture);

		if (allow_render_dest)
		{
			DescriptorAllocation alloc = m_allocators[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->Allocate(6);

			if (alloc.IsNull())
			{
				LOGC("Couldn't allocate descriptor for the texture resource");
			}

			texture->m_rtv_allocation = std::move(alloc);

			d3d12::CreateRTVFromCubemap(texture);
		}

		m_loaded_textures++;

		uint64_t texture_id = m_id_factory.GetUnusedID();

		TextureHandle texture_handle;
		texture_handle.m_pool = this;
		texture_handle.m_id = texture_id;

		m_staged_textures.insert(std::make_pair(texture_id, texture));

		return texture_handle;
	}

	DescriptorAllocator * D3D12TexturePool::GetAllocator(DescriptorHeapType type)
	{
		return m_allocators[static_cast<size_t>(type)];
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
		desc.m_texture_format = (srgb) ? static_cast<wr::Format>(DirectX::MakeSRGB(metadata.format)) : static_cast<wr::Format>(metadata.format);
		desc.m_initial_state = ResourceState::COPY_DEST;

		auto texture = d3d12::CreateTexture(device, &desc, generate_mips);

		texture->m_allocated_memory = static_cast<uint8_t*>(malloc(texture->m_needed_memory));

		memcpy(texture->m_allocated_memory, image.GetPixels(), image.GetPixelsSize());

		DescriptorAllocation alloc = m_allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Allocate();

		if (alloc.IsNull())
		{
			LOGC("Couldn't allocate descriptor for the texture resource");
		}

		texture->m_need_mips = generate_mips;
		texture->m_srv_allocation = std::move(alloc);
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

		bool mip_generation = (metadata.mipLevels > 1) ? false : generate_mips;

		if (mip_generation)
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

		auto texture = d3d12::CreateTexture(device, &desc, mip_generation);

		texture->m_allocated_memory = static_cast<uint8_t*>(malloc(texture->m_needed_memory));

		memcpy(texture->m_allocated_memory, image.GetPixels(), image.GetPixelsSize());

		DescriptorAllocation alloc = m_allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Allocate();

		if (alloc.IsNull())
		{
			LOGC("Couldn't allocate descriptor for the texture resource");
		}

		texture->m_need_mips = mip_generation;
		texture->m_srv_allocation = std::move(alloc);
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

		texture->m_allocated_memory = static_cast<uint8_t*>(malloc(texture->m_needed_memory));

		memcpy(texture->m_allocated_memory, image.GetPixels(), image.GetPixelsSize());

		DescriptorAllocation alloc = m_allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Allocate();

		if (alloc.IsNull())
		{
			LOGC("Couldn't allocate descriptor for the texture resource");
		}

		texture->m_need_mips = generate_mips;
		texture->m_srv_allocation = std::move(alloc);
		texture->m_resource->SetName(wide_string.c_str());

		d3d12::CreateSRVFromTexture(texture);

		m_loaded_textures++;

		return texture;
	}

	d3d12::TextureResource * D3D12TexturePool::LoadPNGFromMemory(char * data, size_t size, bool srgb, bool generate_mips)
	{
		auto device = m_render_system.m_device;

		DirectX::TexMetadata metadata;
		DirectX::ScratchImage image;

		HRESULT hr = DirectX::LoadFromWICMemory(data, size, DirectX::WIC_FLAGS_NONE, &metadata, image);

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
		desc.m_texture_format = (srgb) ? static_cast<wr::Format>(DirectX::MakeSRGB(metadata.format)) : static_cast<wr::Format>(metadata.format);
		desc.m_initial_state = ResourceState::COPY_DEST;

		auto texture = d3d12::CreateTexture(device, &desc, generate_mips);

		texture->m_allocated_memory = static_cast<uint8_t*>(malloc(texture->m_needed_memory));

		memcpy(texture->m_allocated_memory, image.GetPixels(), image.GetPixelsSize());

		DescriptorAllocation alloc = m_allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Allocate();

		if (alloc.IsNull())
		{
			LOGC("Couldn't allocate descriptor for the texture resource");
		}

		texture->m_need_mips = generate_mips;
		texture->m_srv_allocation = std::move(alloc);

		d3d12::CreateSRVFromTexture(texture);

		m_loaded_textures++;

		return texture;
	}

	d3d12::TextureResource * D3D12TexturePool::LoadDDSFromMemory(char * data, size_t size, bool srgb, bool generate_mips)
	{
		auto device = m_render_system.m_device;

		DirectX::TexMetadata metadata;
		DirectX::ScratchImage image;
		
		HRESULT hr = DirectX::LoadFromDDSMemory(data, size,
			DirectX::DDS_FLAGS_NONE, &metadata, image);

		if (FAILED(hr))
		{
			LOGC("ERROR: Texture not loaded correctly.");
		}

		uint32_t mip_lvls;

		bool mip_generation = (metadata.mipLevels > 1) ? false : generate_mips;

		if (mip_generation)
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

		auto texture = d3d12::CreateTexture(device, &desc, mip_generation);

		texture->m_allocated_memory = static_cast<uint8_t*>(malloc(texture->m_needed_memory));

		memcpy(texture->m_allocated_memory, image.GetPixels(), image.GetPixelsSize());

		DescriptorAllocation alloc = m_allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Allocate();

		if (alloc.IsNull())
		{
			LOGC("Couldn't allocate descriptor for the texture resource");
		}

		texture->m_need_mips = mip_generation;
		texture->m_srv_allocation = std::move(alloc);

		d3d12::CreateSRVFromTexture(texture);

		m_loaded_textures++;

		return texture;
	}

	d3d12::TextureResource * D3D12TexturePool::LoadHDRFromMemory(char * data, size_t size, bool srgb, bool generate_mips)
	{
		auto device = m_render_system.m_device;

		DirectX::TexMetadata metadata;
		DirectX::ScratchImage image;

		HRESULT hr = DirectX::LoadFromHDRMemory(data, size, &metadata, image);

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

		texture->m_allocated_memory = static_cast<uint8_t*>(malloc(texture->m_needed_memory));

		memcpy(texture->m_allocated_memory, image.GetPixels(), image.GetPixelsSize());

		DescriptorAllocation alloc = m_allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Allocate();

		if (alloc.IsNull())
		{
			LOGC("Couldn't allocate descriptor for the texture resource");
		}

		texture->m_need_mips = generate_mips;
		texture->m_srv_allocation = std::move(alloc);

		d3d12::CreateSRVFromTexture(texture);

		m_loaded_textures++;

		return texture;
	}

	d3d12::TextureResource * D3D12TexturePool::LoadRawFromMemory(char * data, int width, int height, bool srgb, bool generate_mips)
	{
		auto device = m_render_system.m_device;

		uint32_t mip_lvls;

		if (generate_mips)
		{
			mip_lvls = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
		}
		else
		{
			mip_lvls = 1;
		}

		d3d12::desc::TextureDesc desc;

		desc.m_width = width;
		desc.m_height = height;
		desc.m_is_cubemap = false;
		desc.m_depth = 1;
		desc.m_array_size = 1;
		desc.m_mip_levels = mip_lvls;
		desc.m_texture_format = srgb ? wr::Format::R8G8B8A8_UNORM_SRGB : wr::Format::R8G8B8A8_UNORM;
		desc.m_initial_state = ResourceState::COPY_DEST;

		auto texture = d3d12::CreateTexture(device, &desc, generate_mips);

		texture->m_allocated_memory = static_cast<uint8_t*>(malloc(texture->m_needed_memory));

		memcpy(texture->m_allocated_memory, data, width*height * 4);

		DescriptorAllocation alloc = m_allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Allocate();

		if (alloc.IsNull())
		{
			LOGC("Couldn't allocate descriptor for the texture resource");
		}

		texture->m_need_mips = generate_mips;
		texture->m_srv_allocation = std::move(alloc);

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

	void D3D12TexturePool::GenerateMips(d3d12::TextureResource* texture, CommandList* cmd_list)
	{
		wr::d3d12::CommandList* d3d12_cmd_list = static_cast<wr::d3d12::CommandList*>(cmd_list);
		D3D12Pipeline* pipeline = static_cast<D3D12Pipeline*>(PipelineRegistry::Get().Find(pipelines::mip_mapping));
		auto device = m_render_system.m_device;

		d3d12::BindComputePipeline(d3d12_cmd_list, pipeline->m_native);

		if (texture->m_need_mips)
		{
			if (d3d12::CheckUAVCompatibility(texture->m_format))
			{
				GenerateMips_UAV(texture, cmd_list);
			}
			else if (d3d12::CheckBGRFormat(texture->m_format))
			{
				GenerateMips_UAV(texture, cmd_list);
			}
			else if (d3d12::CheckSRGBFormat(texture->m_format))
			{
				GenerateMips_UAV(texture, cmd_list);
			}
			else
			{
				LOGC("[ERROR]: GenerateMips-> I don't know how we ended up here!");
			}
		}
	}

	void D3D12TexturePool::GenerateMips(std::vector<d3d12::TextureResource*>& const textures, CommandList* cmd_list)
	{
		wr::d3d12::CommandList* d3d12_cmd_list = static_cast<wr::d3d12::CommandList*>(cmd_list);

		D3D12Pipeline* pipeline = static_cast<D3D12Pipeline*>(PipelineRegistry::Get().Find(pipelines::mip_mapping));

		auto device = m_render_system.m_device;

		d3d12::BindComputePipeline(d3d12_cmd_list, pipeline->m_native);

		d3d12::Transition(d3d12_cmd_list, textures, ResourceState::PIXEL_SHADER_RESOURCE, ResourceState::UNORDERED_ACCESS);

		for (size_t i = 0; i < textures.size(); ++i)
		{
			d3d12::TextureResource* texture = textures[i];

			if (texture->m_need_mips)
			{
				if (d3d12::CheckUAVCompatibility(texture->m_format))
				{
					GenerateMips_UAV(texture, cmd_list);
				}
				else if (d3d12::CheckOptionalUAVFormat(texture->m_format))
				{
					GenerateMips_UAV(texture, cmd_list);
				}
				else if (d3d12::CheckBGRFormat(texture->m_format))
				{
					if (d3d12::CheckSRGBFormat(texture->m_format))
					{
						GenerateMips_SRGB(texture, cmd_list);
					}
					else
					{
						GenerateMips_UAV(texture, cmd_list);
					}
				}
				else if (d3d12::CheckSRGBFormat(texture->m_format))
				{
					GenerateMips_SRGB(texture, cmd_list);
				}
				else
				{
					LOGC("[ERROR]: GenerateMips-> I don't know how we ended up here!");
				}
			}
		}

		d3d12::Transition(d3d12_cmd_list, textures, ResourceState::UNORDERED_ACCESS, ResourceState::PIXEL_SHADER_RESOURCE);
	}

	void D3D12TexturePool::GenerateMips_UAV(d3d12::TextureResource* texture, CommandList* cmd_list)
	{
		wr::d3d12::CommandList* d3d12_cmd_list = static_cast<wr::d3d12::CommandList*>(cmd_list);

		for (uint32_t TopMip = 0; TopMip < texture->m_mip_levels - 1; TopMip++)
		{
			uint32_t srcWidth = texture->m_width >> TopMip;
			uint32_t srcHeight = texture->m_height >> TopMip;
			uint32_t dstWidth = srcWidth >> 1;
			uint32_t dstHeight = srcHeight >> 1;

			//Create shader resource view for the source texture in the descriptor heap
			DescriptorAllocation srv_alloc = m_mipmapping_allocator->Allocate();
			d3d12::DescHeapCPUHandle srv_handle = srv_alloc.GetDescriptorHandle();

			d3d12::CreateSRVFromTexture(texture, srv_handle, 1, TopMip);

			//Create unordered access view for the destination texture in the descriptor heap
			DescriptorAllocation uav_alloc = m_mipmapping_allocator->Allocate();
			d3d12::DescHeapCPUHandle uav_handle = uav_alloc.GetDescriptorHandle();

			d3d12::CreateUAVFromTexture(texture, uav_handle, TopMip + 1);

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

			//Pass the source and destination texture views to the shader
			d3d12::SetShaderSRV(d3d12_cmd_list, 1, 0, srv_handle);
			d3d12::SetShaderUAV(d3d12_cmd_list, 1, 1, uav_handle);

			//Dispatch the compute shader with one thread per 8x8 pixels
			d3d12::Dispatch(n_cmd_list, std::max(dstWidth / 8, 1u), std::max(dstHeight / 8, 1u), 1);

			//Wait for all accesses to the destination texture UAV to be finished before generating the next mipmap, as it will be the source texture for the next mipmap
			d3d12::UAVBarrier(n_cmd_list, texture, 1);
		}

		texture->m_need_mips = false;
	}

	void D3D12TexturePool::GenerateMips_BGR(d3d12::TextureResource* texture, CommandList* cmd_list)
	{
	}

	void D3D12TexturePool::GenerateMips_SRGB(d3d12::TextureResource* texture, CommandList* cmd_list)
	{
		auto device = m_render_system.m_device;
		wr::d3d12::CommandList* d3d12_cmd_list = static_cast<wr::d3d12::CommandList*>(cmd_list);

		//Create copy of the texture and store it for later deletion.
		d3d12::desc::TextureDesc copy_desc;

		copy_desc.m_width = texture->m_width;
		copy_desc.m_height = texture->m_height;
		copy_desc.m_is_cubemap = texture->m_is_cubemap;
		copy_desc.m_depth = texture->m_depth;
		copy_desc.m_array_size = texture->m_array_size;
		copy_desc.m_mip_levels = texture->m_mip_levels;
		copy_desc.m_texture_format = d3d12::RemoveSRGB(texture->m_format);
		copy_desc.m_initial_state = ResourceState::COPY_DEST;

		auto texture_copy = d3d12::CreateTexture(device, &copy_desc, true);

		m_temporary_textures.push_back(texture_copy);

		d3d12::CopyResource(d3d12_cmd_list, texture, texture_copy);
		
		GenerateMips_UAV(texture_copy, cmd_list);

		d3d12::CopyResource(d3d12_cmd_list, texture_copy, texture);
	}
}
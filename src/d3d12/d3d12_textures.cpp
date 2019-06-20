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
#include "d3d12_functions.hpp"

#include "../util/log.hpp"
#include "d3d12_defines.hpp"
#include "d3dx12.hpp"
#include "d3d12_texture_resources.hpp"
#include "d3d12_rt_descriptor_heap.hpp"

namespace wr::d3d12
{
	TextureResource* CreateTexture(Device* device, desc::TextureDesc* description, bool allow_uav)
	{
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

		bool is_uav_compatible_format = (d3d12::CheckUAVCompatibility(description->m_texture_format) 
									     || d3d12::CheckOptionalUAVFormat(description->m_texture_format));

		if (allow_uav && is_uav_compatible_format)
		{ 
			flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; 
		}

		if (description->m_initial_state == ResourceState::RENDER_TARGET) { flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET; }

		D3D12_RESOURCE_DESC desc = {};
		desc.Width = description->m_width;
		desc.Height = description->m_height;
		desc.MipLevels = static_cast<std::uint16_t>(description->m_mip_levels);
		desc.DepthOrArraySize = static_cast<UINT16>((description->m_depth > 1) ? description->m_depth : description->m_array_size);
		desc.Format = static_cast<DXGI_FORMAT>(description->m_texture_format);
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Flags = flags;
		desc.Dimension = (description->m_depth > 1) ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

		auto native_device = device->m_native;

		ID3D12Resource* resource;

		HRESULT res = native_device->CreateCommittedResource(&defaultHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			static_cast<D3D12_RESOURCE_STATES>(description->m_initial_state),
			nullptr,
			IID_PPV_ARGS(&resource));

		if (FAILED(res))
		{
			LOGC("Error: Couldn't create texture");
		}

		// Create intermediate resource on upload heap for staging
		uint64_t textureUploadBufferSize;
		device->m_native->GetCopyableFootprints(&desc, 0, desc.MipLevels * desc.DepthOrArraySize, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

		ID3D12Resource* intermediate;

		CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize);

		device->m_native->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&buffer_desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&intermediate));


		TextureResource* texture = new TextureResource();

		texture->m_width = description->m_width;
		texture->m_height = description->m_height;
		texture->m_depth = description->m_depth;
		texture->m_array_size = description->m_array_size;
		texture->m_mip_levels = description->m_mip_levels;
		texture->m_format = description->m_texture_format;
		texture->m_resource = resource;
		texture->m_intermediate = intermediate;
		texture->m_need_mips = (texture->m_mip_levels > 1);
		texture->m_is_cubemap = description->m_is_cubemap;
		texture->m_is_staged = false;

		for (uint32_t i = 0; i < description->m_mip_levels; ++i)
		{
			texture->m_subresource_states.push_back(description->m_initial_state);
		}

		return texture;
	}

	TextureResource* CreatePlacedTexture(Device* device, desc::TextureDesc* description, bool allow_uav, Heap<HeapOptimization::BIG_STATIC_BUFFERS>* heap)
	{
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

		bool is_uav_compatible_format = (d3d12::CheckUAVCompatibility(description->m_texture_format)
			|| d3d12::CheckOptionalUAVFormat(description->m_texture_format));

		if (allow_uav && is_uav_compatible_format)
		{
			flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

		if (description->m_initial_state == ResourceState::RENDER_TARGET) { flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET; }

		D3D12_RESOURCE_DESC desc = {};
		desc.Width = description->m_width;
		desc.Height = description->m_height;
		desc.MipLevels = static_cast<std::uint16_t>(description->m_mip_levels);
		desc.DepthOrArraySize = static_cast<std::uint16_t>((description->m_depth > 1) ? description->m_depth : description->m_array_size);
		desc.Format = static_cast<DXGI_FORMAT>(description->m_texture_format);
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Flags = flags;
		desc.Dimension = (description->m_depth > 1) ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;


		auto native_device = device->m_native;

		ID3D12Resource* resource;

		HRESULT res = native_device->CreatePlacedResource(
			heap->m_native, 
			0,
			&desc,
			static_cast<D3D12_RESOURCE_STATES>(description->m_initial_state),
			nullptr,
			IID_PPV_ARGS(&resource));

		if (FAILED(res))
		{
			LOGC("Error: Couldn't create texture");
		}

		TextureResource* texture = new TextureResource();

		texture->m_width = description->m_width;
		texture->m_height = description->m_height;
		texture->m_depth = description->m_depth;
		texture->m_array_size = description->m_array_size;
		texture->m_mip_levels = description->m_mip_levels;
		texture->m_format = description->m_texture_format;
		texture->m_resource = resource;
		texture->m_intermediate = nullptr;
		texture->m_need_mips = (texture->m_mip_levels > 1);
		texture->m_is_cubemap = description->m_is_cubemap;
		texture->m_is_staged = false;

		for (uint32_t i = 0; i < description->m_mip_levels; ++i)
		{
			texture->m_subresource_states.push_back(description->m_initial_state);
		}

		return texture;
	}

	void SetName(TextureResource* tex, std::wstring name)
	{
		tex->m_resource->SetName(name.c_str());
	}

	void CreateSRVFromTexture(TextureResource* tex)
	{
		d3d12::DescHeapCPUHandle handle = tex->m_srv_allocation.GetDescriptorHandle();

		CreateSRVFromTexture(tex, handle);
	}

	void CreateSRVFromTexture(TextureResource* tex, DescHeapCPUHandle& handle)
	{
		decltype(Device::m_native) n_device;

		tex->m_resource->GetDevice(IID_PPV_ARGS(&n_device));

		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.Format = (DXGI_FORMAT)tex->m_format;

		//Calculate dimension
		D3D12_SRV_DIMENSION dimension;

		if (tex->m_is_cubemap)
		{
			dimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			srv_desc.TextureCube.MipLevels = static_cast<std::uint32_t>(tex->m_mip_levels);
		}
		else
		{
			if (tex->m_depth > 1)
			{
				//Then it's a 3D texture
				dimension = D3D12_SRV_DIMENSION_TEXTURE3D;
				srv_desc.Texture3D.MipLevels = static_cast<std::uint32_t>(tex->m_mip_levels);
			}
			else
			{
				if (tex->m_height > 1)
				{
					if (tex->m_array_size > 1)
					{
						dimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
						srv_desc.Texture2DArray.MipLevels = static_cast<std::uint32_t>(tex->m_mip_levels);
						srv_desc.Texture2DArray.ArraySize = static_cast<std::uint32_t>(tex->m_array_size);
					}
					else
					{
						dimension = D3D12_SRV_DIMENSION_TEXTURE2D;
						srv_desc.Texture2D.MipLevels = static_cast<std::uint32_t>(tex->m_mip_levels);
					}
				}
				else
				{
					//Then it's a 1D texture
					if (tex->m_array_size > 1)
					{
						dimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;

						srv_desc.Texture1DArray.MipLevels = static_cast<std::uint32_t>(tex->m_mip_levels);
						srv_desc.Texture1DArray.ArraySize = static_cast<std::uint32_t>(tex->m_array_size);
					}
					else
					{
						dimension = D3D12_SRV_DIMENSION_TEXTURE1D;

						srv_desc.Texture1D.MipLevels = static_cast<std::uint32_t>(tex->m_mip_levels);
					}
				}
			}

		}

		srv_desc.ViewDimension = dimension;

		n_device->CreateShaderResourceView(tex->m_resource, &srv_desc, handle.m_native);
	}

	void CreateSRVFromTexture(TextureResource* tex, DescHeapCPUHandle& handle, unsigned int mip_levels, unsigned int most_detailed_mip)
	{
		decltype(Device::m_native) n_device;

		tex->m_resource->GetDevice(IID_PPV_ARGS(&n_device));

		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.Format = (DXGI_FORMAT)tex->m_format;

		//Calculate dimension
		D3D12_SRV_DIMENSION dimension;

		if (tex->m_is_cubemap)
		{
			dimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			srv_desc.TextureCube.MipLevels = mip_levels;
			srv_desc.TextureCube.MostDetailedMip = most_detailed_mip;
		}
		else
		{
			if (tex->m_depth > 1)
			{
				//Then it's a 3D texture
				dimension = D3D12_SRV_DIMENSION_TEXTURE3D;
				srv_desc.Texture3D.MipLevels = mip_levels;
				srv_desc.Texture3D.MostDetailedMip = most_detailed_mip;
			}
			else
			{
				if (tex->m_height > 1)
				{
					if (tex->m_array_size > 1)
					{
						dimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
						srv_desc.Texture2DArray.MipLevels = mip_levels;
						srv_desc.Texture2DArray.MostDetailedMip = most_detailed_mip;
						srv_desc.Texture2DArray.ArraySize = static_cast<std::uint32_t>(tex->m_array_size);
					}
					else
					{
						dimension = D3D12_SRV_DIMENSION_TEXTURE2D;
						srv_desc.Texture2D.MipLevels = mip_levels;
						srv_desc.Texture2D.MostDetailedMip = most_detailed_mip;
					}
				}
				else
				{
					//Then it's a 1D texture
					if (tex->m_array_size > 1)
					{
						dimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;

						srv_desc.Texture1DArray.MipLevels = mip_levels;
						srv_desc.Texture1DArray.MostDetailedMip = most_detailed_mip;
						srv_desc.Texture1DArray.ArraySize = static_cast<std::uint32_t>(tex->m_array_size);
					}
					else
					{
						dimension = D3D12_SRV_DIMENSION_TEXTURE1D;

						srv_desc.Texture1D.MipLevels = mip_levels;
						srv_desc.Texture1D.MostDetailedMip = most_detailed_mip;
					}
				}
			}

		}

		srv_desc.ViewDimension = dimension;

		n_device->CreateShaderResourceView(tex->m_resource, &srv_desc, handle.m_native);
	}

	void CreateSRVFromCubemapFace(TextureResource* tex, DescHeapCPUHandle& handle, unsigned int mip_levels, unsigned int most_detailed_mip, unsigned int face_idx)
	{
		decltype(Device::m_native) n_device;

		tex->m_resource->GetDevice(IID_PPV_ARGS(&n_device));

		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.Format = (DXGI_FORMAT)tex->m_format;

		//Calculate dimension
		D3D12_SRV_DIMENSION dimension;

		if (tex->m_is_cubemap)
		{
			dimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			srv_desc.Texture2DArray.MipLevels = mip_levels;
			srv_desc.Texture2DArray.MostDetailedMip = most_detailed_mip;
			srv_desc.Texture2DArray.ArraySize = 1;
			srv_desc.Texture2DArray.FirstArraySlice = face_idx;
		}
		else
		{
			LOGC("[ERROR]: Texture is not a cubemap");
			return;
		}

		srv_desc.ViewDimension = dimension;

		n_device->CreateShaderResourceView(tex->m_resource, &srv_desc, handle.m_native);
	}

	void CreateUAVFromTexture(TextureResource* tex, unsigned int mip_slice)
	{
		d3d12::DescHeapCPUHandle handle = tex->m_uav_allocation.GetDescriptorHandle();

		CreateUAVFromTexture(tex, handle, mip_slice);
	}

	void CreateUAVFromTexture(TextureResource* tex, DescHeapCPUHandle& handle, unsigned int mip_slice)
	{
		decltype(Device::m_native) n_device;

		tex->m_resource->GetDevice(IID_PPV_ARGS(&n_device));

		D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
		
		uav_desc.Format = (DXGI_FORMAT)tex->m_format;

		//Calculate dimension
		D3D12_UAV_DIMENSION dimension;

		if (tex->m_is_cubemap)
		{
			dimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			uav_desc.Texture2DArray.MipSlice = mip_slice;
			uav_desc.Texture2DArray.ArraySize = static_cast<std::uint32_t>(tex->m_array_size);
		}
		else
		{
			if (tex->m_depth > 1)
			{
				//Then it's a 3D texture
				dimension = D3D12_UAV_DIMENSION_TEXTURE3D;

				uav_desc.Texture3D.MipSlice = mip_slice;
			}
			else
			{
				if (tex->m_height > 1)
				{
					if (tex->m_array_size > 1)
					{
						dimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;

						uav_desc.Texture2DArray.MipSlice = mip_slice;
						uav_desc.Texture2DArray.ArraySize = static_cast<std::uint32_t>(tex->m_array_size);
					}
					else
					{
						dimension = D3D12_UAV_DIMENSION_TEXTURE2D;

						uav_desc.Texture2D.MipSlice = mip_slice;
					}
				}
				else
				{
					//Then it's a 1D texture
					if (tex->m_array_size > 1)
					{
						dimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;

						uav_desc.Texture1DArray.MipSlice = mip_slice;
						uav_desc.Texture1DArray.ArraySize = static_cast<std::uint32_t>(tex->m_array_size);
					}
					else
					{
						dimension = D3D12_UAV_DIMENSION_TEXTURE1D;

						uav_desc.Texture1D.MipSlice = mip_slice;
					}
				}
			}

		}

		uav_desc.ViewDimension = dimension;

		n_device->CreateUnorderedAccessView(tex->m_resource, nullptr, &uav_desc, handle.m_native);
	}

	void CreateUAVFromCubemapFace(TextureResource * tex, DescHeapCPUHandle & handle, unsigned int mip_slice, unsigned int face_idx)
	{
		decltype(Device::m_native) n_device;

		tex->m_resource->GetDevice(IID_PPV_ARGS(&n_device));

		D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};

		uav_desc.Format = (DXGI_FORMAT)tex->m_format;

		if (tex->m_is_cubemap)
		{
			uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			uav_desc.Texture2DArray.MipSlice = mip_slice;
			uav_desc.Texture2DArray.FirstArraySlice = face_idx;
			uav_desc.Texture2DArray.ArraySize = 1;
		}
		else
		{
			LOGC("[ERROR]: Texture is not a cubemap");
		}

		n_device->CreateUnorderedAccessView(tex->m_resource, nullptr, &uav_desc, handle.m_native);
	}

	void CreateRTVFromTexture2D(TextureResource* tex)
	{
		decltype(Device::m_native) n_device;

		tex->m_resource->GetDevice(IID_PPV_ARGS(&n_device));


		if (tex->m_rtv_allocation == std::nullopt)
		{
			LOGC("This texture was not created with the intent of being used as a RTV.");
		}

		D3D12_RENDER_TARGET_VIEW_DESC desc;
		desc.Format = static_cast<DXGI_FORMAT>(tex->m_format);
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;

		DescHeapCPUHandle handle = tex->m_rtv_allocation->GetDescriptorHandle();

		n_device->CreateRenderTargetView(tex->m_resource, &desc, handle.m_native);
	}

	void CreateRTVFromCubemap(TextureResource* tex)
	{
		decltype(Device::m_native) n_device;

		tex->m_resource->GetDevice(IID_PPV_ARGS(&n_device));

		if (!tex->m_is_cubemap)
		{
			LOGC("This texture is not a cubemap.");
		}

		if (tex->m_rtv_allocation == std::nullopt)
		{
			LOGC("This texture was not created with the intent of being used as a RTV.");
		}

		for (uint8_t i = 0; i < 6; ++i)
		{
			D3D12_RENDER_TARGET_VIEW_DESC desc;
			desc.Format = static_cast<DXGI_FORMAT>(tex->m_format);
			desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			desc.Texture2DArray.MipSlice = 0;
			desc.Texture2DArray.PlaneSlice = 0;

			//Render target to ith element
			desc.Texture2DArray.FirstArraySlice = i;

			//Only view one element of the array
			desc.Texture2DArray.ArraySize = 1;

			DescHeapCPUHandle handle = tex->m_rtv_allocation->GetDescriptorHandle(i);

			n_device->CreateRenderTargetView(tex->m_resource, &desc, handle.m_native);
		}
	}

	void SetShaderSRV(wr::d3d12::CommandList* cmd_list, uint32_t rootParameterIndex, uint32_t descriptorOffset, TextureResource* tex)
	{
		d3d12::DescHeapCPUHandle handle = tex->m_srv_allocation.GetDescriptorHandle();

		cmd_list->m_dynamic_descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIndex, descriptorOffset, 1, handle);
	}

	void SetShaderSRV(wr::d3d12::CommandList* cmd_list, uint32_t root_parameter_index, uint32_t descriptor_offset, d3d12::DescHeapCPUHandle& handle, uint32_t descriptor_count)
	{
		cmd_list->m_dynamic_descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(root_parameter_index, descriptor_offset, descriptor_count, handle);
	}

	void SetShaderUAV(wr::d3d12::CommandList* cmd_list, uint32_t rootParameterIndex, uint32_t descriptorOffset, TextureResource* tex)
	{
		d3d12::DescHeapCPUHandle handle = tex->m_uav_allocation.GetDescriptorHandle();

		cmd_list->m_dynamic_descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIndex, descriptorOffset, 1, handle);
	}

	void SetShaderUAV(wr::d3d12::CommandList* cmd_list, uint32_t root_parameter_index, uint32_t descriptor_offset, d3d12::DescHeapCPUHandle& handle, uint32_t descriptor_count)
	{
		cmd_list->m_dynamic_descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(root_parameter_index, descriptor_offset, descriptor_count, handle);
	}

	void SetRTShaderSRV(wr::d3d12::CommandList* cmd_list, uint32_t rootParameterIndex, uint32_t descriptorOffset, TextureResource* tex)
	{
		d3d12::DescHeapCPUHandle handle = tex->m_srv_allocation.GetDescriptorHandle();

		cmd_list->m_rt_descriptor_heap->StageDescriptors(rootParameterIndex, descriptorOffset, 1, handle);
	}

	void SetRTShaderSRV(wr::d3d12::CommandList* cmd_list, uint32_t root_parameter_index, uint32_t descriptor_offset, d3d12::DescHeapCPUHandle& handle, uint32_t descriptor_count)
	{
		cmd_list->m_rt_descriptor_heap->StageDescriptors(root_parameter_index, descriptor_offset, descriptor_count, handle);
	}

	void SetRTShaderUAV(wr::d3d12::CommandList* cmd_list, uint32_t rootParameterIndex, uint32_t descriptorOffset, TextureResource* tex)
	{
		d3d12::DescHeapCPUHandle handle = tex->m_uav_allocation.GetDescriptorHandle();

		cmd_list->m_rt_descriptor_heap->StageDescriptors(rootParameterIndex, descriptorOffset, 1, handle);
	}

	void SetRTShaderUAV(wr::d3d12::CommandList* cmd_list, uint32_t root_parameter_index, uint32_t descriptor_offset, d3d12::DescHeapCPUHandle& handle, uint32_t descriptor_count)
	{
		cmd_list->m_rt_descriptor_heap->StageDescriptors(root_parameter_index, descriptor_offset, descriptor_count, handle);
	}

	void CopyResource(wr::d3d12::CommandList* cmd_list, TextureResource* src_texture, TextureResource* dst_texture)
	{
		ResourceState src_original_state = src_texture->m_subresource_states[0];
		ResourceState dst_original_state = dst_texture->m_subresource_states[0];

		d3d12::Transition(cmd_list, src_texture, src_original_state, ResourceState::COPY_SOURCE);
		d3d12::Transition(cmd_list, dst_texture, dst_original_state, ResourceState::COPY_DEST);

		cmd_list->m_native->CopyResource(dst_texture->m_resource, src_texture->m_resource);

		d3d12::Transition(cmd_list, src_texture, ResourceState::COPY_SOURCE, src_original_state);
		d3d12::Transition(cmd_list, dst_texture, ResourceState::COPY_DEST, dst_original_state);
	}

	void Destroy(TextureResource* tex)
	{
		if (tex != nullptr)
		{
			SAFE_RELEASE(tex->m_resource);
			delete tex;
		}
	}

	bool CheckUAVCompatibility(Format format)
	{
		switch (format)
		{
		case Format::R32G32B32A32_FLOAT:
		case Format::R32G32B32A32_UINT:
		case Format::R32G32B32A32_SINT:
		case Format::R16G16B16A16_FLOAT:
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

	bool CheckOptionalUAVFormat(Format format)
	{
		switch (format)
		{
		case Format::R16G16B16A16_UNORM:
		case Format::R16G16B16A16_SNORM:
		case Format::R32G32_FLOAT:
		case Format::R32G32_UINT:
		case Format::R32G32_SINT:
		case Format::R10G10B10A2_UNORM:
		case Format::R10G10B10A2_UINT:
		case Format::R11G11B10_FLOAT:
		case Format::R8G8B8A8_SNORM:
		case Format::R16G16_FLOAT:
		case Format::R16G16_UNORM:
		case Format::R16G16_UINT:
		case Format::R16G16_SNORM:
		case Format::R16G16_SINT:
		case Format::R8G8_UNORM:
		case Format::R8G8_UINT:
		case Format::R8G8_SNORM:
		case Format::R8G8_SINT:
		case Format::R16_UNORM:
		case Format::R16_SNORM:
		case Format::R8_SNORM:
		case Format::A8_UNORM:
		case Format::B5G6R5_UNORM:
		case Format::B5G5R5A1_UNORM:
		case Format::B4G4R4A4_UNORM:
			return true;
		default:
			return false;
		}
	}

	bool CheckBGRFormat(Format format)
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

	bool CheckSRGBFormat(Format format)
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

	bool IsOptionalFormatSupported(Device* device, Format format)
	{
		return device->m_optional_formats.test(static_cast<DXGI_FORMAT>(format));
	}

	Format RemoveSRGB(Format format)
	{
		Format out_format = Format::UNKNOWN;

		switch (format)
		{
		case wr::Format::R8G8B8A8_UNORM_SRGB:
			out_format = Format::R8G8B8A8_UNORM;
			break;		
		case wr::Format::B8G8R8A8_UNORM_SRGB:
			out_format = Format::B8G8R8A8_UNORM;
			break;
		case wr::Format::B8G8R8X8_UNORM_SRGB:
			out_format = Format::B8G8R8X8_UNORM;
			break;
		default:
			break;
		}

		return out_format;
	}

	Format BGRtoRGB(Format format)
	{
		Format out_format = Format::UNKNOWN;

		switch (format)
		{
		case Format::B8G8R8A8_UNORM:
			out_format = Format::R8G8B8A8_UNORM;
			break;
		case Format::B8G8R8X8_UNORM:
			out_format = Format::R8G8B8A8_UNORM;
			break;
		case Format::B8G8R8A8_UNORM_SRGB:
			out_format = Format::R8G8B8A8_UNORM_SRGB;
			break;
		case Format::B8G8R8X8_UNORM_SRGB:
			out_format = Format::R8G8B8A8_UNORM_SRGB;
			break;
		default:
			break;
		}

		return out_format;
	}

} /* wr::d3d12 */
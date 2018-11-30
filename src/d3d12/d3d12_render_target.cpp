#include "d3d12_functions.hpp"

#include "../util/log.hpp"
#include "d3d12_defines.hpp"
#include "d3dx12.hpp"

namespace wr::d3d12
{
	
	RenderTarget* CreateRenderTarget(Device* device, unsigned int width, unsigned int height, desc::RenderTargetDesc descriptor)
	{
		auto render_target = new RenderTarget();
		const auto n_device = device->m_native;

		render_target->m_render_targets.resize(descriptor.m_num_rtv_formats);
		render_target->m_create_info = descriptor;
		render_target->m_num_render_targets = descriptor.m_num_rtv_formats;

		for (auto i = 0; i < descriptor.m_num_rtv_formats; i++)
		{
			CD3DX12_RESOURCE_DESC resource_desc = CD3DX12_RESOURCE_DESC::Tex2D((DXGI_FORMAT)descriptor.m_rtv_formats[i], width, height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

			D3D12_CLEAR_VALUE optimized_clear_value = {
				(DXGI_FORMAT)descriptor.m_rtv_formats[i],
				descriptor.m_clear_color[0],
				descriptor.m_clear_color[1],
				descriptor.m_clear_color[2],
				descriptor.m_clear_color[3]
			};

			// Create default heap
			TRY_M(n_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&resource_desc,
				(D3D12_RESOURCE_STATES)descriptor.m_initial_state,
				&optimized_clear_value, // optimizes draw call
				IID_PPV_ARGS(&render_target->m_render_targets[i])
			), "Failed to create render target.");

			NAME_D3D12RESOURCE(render_target->m_render_targets[i], L"Unnamed Render Target (" + std::to_wstring(i).c_str() + L")");
			UINT64 textureUploadBufferSize;
			n_device->GetCopyableFootprints(&resource_desc, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);
		}

		CreateRenderTargetViews(render_target, device, width, height);

		if (descriptor.m_create_dsv_buffer)
		{
			CreateDepthStencilBuffer(render_target, device, width, height);
		}

		return render_target;
	}

	void SetName(RenderTarget* render_target, std::wstring name)
	{
		for (auto i = 0; i < render_target->m_num_render_targets; i++)
		{
			render_target->m_render_targets[i]->SetName((name + std::to_wstring(i)).c_str());
		}
	}

	void SetName(RenderTarget* render_target, std::string name)
	{
		SetName(render_target, std::wstring(name.begin(), name.end()));
	}

	void CreateRenderTargetViews(RenderTarget* render_target, Device* device, unsigned int width, unsigned int height)
	{
		const auto n_device = device->m_native;

		// Create views
		D3D12_DESCRIPTOR_HEAP_DESC back_buffer_heap_desc = {};
		back_buffer_heap_desc.NumDescriptors = render_target->m_num_render_targets;
		back_buffer_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		back_buffer_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		TRY_M(n_device->CreateDescriptorHeap(&back_buffer_heap_desc, IID_PPV_ARGS(&render_target->m_rtv_descriptor_heap)),
			"Failed to create descriptor heap.");

		render_target->m_rtv_descriptor_increment_size = n_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// Create render target view with the handle to the heap descriptor.
		render_target->m_render_targets.resize(render_target->m_num_render_targets);
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(render_target->m_rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart());
		for (auto& rt : render_target->m_render_targets)
		{
			n_device->CreateRenderTargetView(rt, nullptr, rtv_handle);

			rtv_handle.Offset(1, render_target->m_rtv_descriptor_increment_size);
		}
	}

	void CreateDepthStencilBuffer(RenderTarget* render_target, Device* device, unsigned int width, unsigned int height)
	{
		const auto n_device = device->m_native;
		auto depth_format = DXGI_FORMAT_R32_TYPELESS;

		// TODO: Seperate the descriptor heap because that one might not need to be recreated when resizing.
		// create a depth stencil descriptor heap so we can get a pointer to the depth stencil buffer
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		TRY_M(n_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&render_target->m_depth_stencil_resource_heap)),
			"Failed to create descriptor heap for depth buffer");
		NAME_D3D12RESOURCE(render_target->m_depth_stencil_resource_heap)

		D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
		depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

		D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
		depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
		depthOptimizedClearValue.DepthStencil.Stencil = 0;

		TRY_M(n_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(depth_format, width, height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthOptimizedClearValue,
			IID_PPV_ARGS(&render_target->m_depth_stencil_buffer)
		), "Failed to create commited resource.");
		NAME_D3D12RESOURCE(render_target->m_depth_stencil_buffer)

		n_device->CreateDepthStencilView(render_target->m_depth_stencil_buffer, &depthStencilDesc, render_target->m_depth_stencil_resource_heap->GetCPUDescriptorHandleForHeapStart());
	}

	void CreateSRVFromDSV(RenderTarget* render_target, DescHeapCPUHandle& handle)
	{
		decltype(Device::m_native) n_device;
		render_target->m_render_targets[0]->GetDevice(IID_PPV_ARGS(&n_device));

		unsigned int increment_size = n_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Texture2D.MipLevels = 1;

		n_device->CreateShaderResourceView(render_target->m_depth_stencil_buffer, &srv_desc, handle.m_native);
		Offset(handle, 1, increment_size);
	}

	void CreateSRVFromRTV(RenderTarget* render_target, DescHeapCPUHandle& handle, unsigned int num, Format formats[8])
	{
		decltype(Device::m_native) n_device;
		render_target->m_render_targets[0]->GetDevice(IID_PPV_ARGS(&n_device));

		unsigned int increment_size = n_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		for (unsigned int i = 0; i < num; i++)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srv_desc.Format = (DXGI_FORMAT)formats[i];
			srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srv_desc.Texture2D.MipLevels = 1;

			n_device->CreateShaderResourceView(render_target->m_render_targets[i], &srv_desc, handle.m_native);
			Offset(handle, 1, increment_size);
		}
	}

	void CreateSRVFromSpecificRTV(RenderTarget* render_target, DescHeapCPUHandle& handle, unsigned int id, Format format)
	{
		decltype(Device::m_native) n_device;
		render_target->m_render_targets[0]->GetDevice(IID_PPV_ARGS(&n_device));

		unsigned int increment_size = n_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.Format = (DXGI_FORMAT)format;
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Texture2D.MipLevels = 1;

		n_device->CreateShaderResourceView(render_target->m_render_targets[id], &srv_desc, handle.m_native);
		Offset(handle, 1, increment_size);
	}

	void Resize(RenderTarget** render_target, Device* device, unsigned int width, unsigned int height)
	{
		if ((*render_target)->m_create_info.m_dsv_format == Format::UNKNOWN && (*render_target)->m_create_info.m_create_dsv_buffer)
		{
			DestroyDepthStencilBuffer((*render_target));
		}
		DestroyRenderTargetViews((*render_target));

		(*render_target) = CreateRenderTarget(device, width, height, (*render_target)->m_create_info);
	}

	void IncrementFrameIdx(RenderTarget* render_target)
	{
		render_target->m_frame_idx = (render_target->m_frame_idx + 1) % render_target->m_num_render_targets;
	}

	void DestroyDepthStencilBuffer(RenderTarget* render_target)
	{
		SAFE_RELEASE(render_target->m_depth_stencil_buffer);
		SAFE_RELEASE(render_target->m_depth_stencil_resource_heap);
	}

	void DestroyRenderTargetViews(RenderTarget* render_target)
	{
		SAFE_RELEASE(render_target->m_rtv_descriptor_heap);
		render_target->m_frame_idx = 0;
		for (auto i = 0; i < render_target->m_render_targets.size(); i++)
		{
			SAFE_RELEASE(render_target->m_render_targets[i]);
		}
	}

	void Destroy(RenderTarget* render_target)
	{
		if (render_target->m_create_info.m_create_dsv_buffer)
		{
			DestroyDepthStencilBuffer(render_target);
		}
		DestroyRenderTargetViews(render_target);

		delete render_target;
	}


	Texture* CreateTexture(Device* device, desc::TextureDesc* description, bool allow_uav)
	{
		D3D12_RESOURCE_DESC desc = {};
		desc.Width = description->m_width;
		desc.Height = description->m_height;
		desc.MipLevels = static_cast<UINT16>(description->m_mip_levels);
		desc.DepthOrArraySize = (description->m_depth > 1) ? description->m_array_size : description->m_depth;
		desc.Format = static_cast<DXGI_FORMAT>(description->m_texture_format);
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Flags = allow_uav ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

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
		device->m_native->GetCopyableFootprints(&desc, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

		ID3D12Resource* intermediate;

		CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);

		device->m_native->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&intermediate));


		Texture* texture = new Texture();

		texture->m_width = description->m_width;
		texture->m_height = description->m_height;
		texture->m_depth = description->m_depth;
		texture->m_array_size = description->m_array_size;
		texture->m_mip_levels = description->m_mip_levels;
		texture->m_format = description->m_texture_format;
		texture->m_current_state = ResourceState::COPY_DEST;
		texture->m_resource = resource;
		texture->m_intermediate = intermediate;
		texture->m_is_cubemap = description->m_is_cubemap;

		return texture;
	}

	void CreateSRVFromTexture(Texture* tex, DescHeapCPUHandle& handle, Format format)
	{
		decltype(Device::m_native) n_device;

		tex->m_resource->GetDevice(IID_PPV_ARGS(&n_device));

		unsigned int increment_size = n_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.Format = (DXGI_FORMAT)format;
		srv_desc.Texture2D.MipLevels = tex->m_mip_levels;

		//Calculate dimension
		D3D12_SRV_DIMENSION dimension;

		if (tex->m_is_cubemap)
		{
			dimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		}
		else
		{
			if (tex->m_depth > 1)
			{
				//Then it's a 3D texture
				dimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			}
			else
			{
				if (tex->m_height > 1)
				{
					if (tex->m_array_size > 1)
					{
						dimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
					}
					else
					{
						dimension = D3D12_SRV_DIMENSION_TEXTURE2D;
					}
				}
				else
				{
					//Then it's a 1D texture
					if (tex->m_array_size > 1)
					{
						dimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
					}
					else
					{
						dimension = D3D12_SRV_DIMENSION_TEXTURE1D;
					}
				}
			}

		}

		srv_desc.ViewDimension = dimension;

		n_device->CreateShaderResourceView(tex->m_resource, &srv_desc, handle.m_native);
		Offset(handle, 1, increment_size);
	}

	void Destroy(Texture* tex)
	{
		SAFE_RELEASE(tex->m_resource);
		delete tex;
	}


} /* wr::d3d12 */
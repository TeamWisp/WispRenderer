#include "d3d12_functions.hpp"

#include "../util/log.hpp"
#include "d3d12_defines.hpp"
#include "d3dx12.hpp"

namespace wr::d3d12
{

	namespace internal
	{
		template<typename T>
		auto SaveFrameIdx(T frame_idx, RenderTarget* rt)
		{
			return frame_idx & rt->m_versioning_count;
		}
	}
	
	RenderTarget* CreateRenderTarget(Device* device, unsigned int width, unsigned int height, desc::RenderTargetDesc descriptor)
	{
		auto render_target = new RenderTarget();
		const auto n_device = device->m_native;

		render_target->m_versioning_count = descriptor.m_versioning_count;
		render_target->m_render_targets.resize(descriptor.m_num_rtv_formats * descriptor.m_versioning_count);
		render_target->m_create_info = descriptor;
		render_target->m_num_render_targets = descriptor.m_num_rtv_formats;

		for (auto i = 0; i < descriptor.m_num_rtv_formats * descriptor.m_versioning_count; i++)
		{
			CD3DX12_RESOURCE_DESC resource_desc = CD3DX12_RESOURCE_DESC::Tex2D((DXGI_FORMAT)descriptor.m_rtv_formats[i % descriptor.m_num_rtv_formats], width, height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

			D3D12_CLEAR_VALUE optimized_clear_value = {
				(DXGI_FORMAT)descriptor.m_rtv_formats[i % descriptor.m_num_rtv_formats],
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
			HRESULT HR;
			NAME_D3D12RESOURCE(HR = render_target->m_render_targets[i], L"Unnamed Render Target (" + std::to_wstring(i).c_str() + L")");
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
		desc::DescriptorHeapDesc back_buffer_heap_desc;
		back_buffer_heap_desc.m_num_descriptors = render_target->m_num_render_targets;
		back_buffer_heap_desc.m_versions = render_target->m_versioning_count;
		back_buffer_heap_desc.m_shader_visible = false;
		back_buffer_heap_desc.m_type = DescriptorHeapType::DESC_HEAP_TYPE_RTV;
		render_target->m_rtv_descriptor_heap = d3d12::CreateDescriptorHeap(device, back_buffer_heap_desc);

		render_target->m_rtv_descriptor_increment_size = n_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// Create render target view with the handle to the heap descriptor.
		for (auto v = 0; v < render_target->m_versioning_count; v++)
		{
			auto rtv_handle = GetCPUHandle(render_target->m_rtv_descriptor_heap, v);
			for (auto i = 0; i < render_target->m_num_render_targets; i++)
			{
				n_device->CreateRenderTargetView(render_target->m_render_targets[i + (render_target->m_num_render_targets * v)], nullptr, rtv_handle.m_native);

				d3d12::Offset(rtv_handle, 1, render_target->m_rtv_descriptor_increment_size);
			}
		}
	}

	void CreateDepthStencilBuffer(RenderTarget* render_target, Device* device, unsigned int width, unsigned int height)
	{
		const auto n_device = device->m_native;
		auto depth_format = DXGI_FORMAT_R32_TYPELESS;

		// TODO: Seperate the descriptor heap because that one might not need to be recreated when resizing.
		// create a depth stencil descriptor heap so we can get a pointer to the depth stencil buffer
		desc::DescriptorHeapDesc dsv_heap_desc;
		dsv_heap_desc.m_num_descriptors = 1;
		dsv_heap_desc.m_versions = render_target->m_versioning_count;
		dsv_heap_desc.m_shader_visible = false;
		dsv_heap_desc.m_type = DescriptorHeapType::DESC_HEAP_TYPE_DSV;
		render_target->m_depth_stencil_resource_heap = d3d12::CreateDescriptorHeap(device, dsv_heap_desc);

		D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
		depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

		D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
		depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
		depthOptimizedClearValue.DepthStencil.Stencil = 0;

		render_target->m_depth_stencil_buffers.resize(render_target->m_versioning_count);
		for (auto v = 0; v < render_target->m_versioning_count; v++)
		{
			TRY_M(n_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Tex2D(depth_format, width, height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&depthOptimizedClearValue,
				IID_PPV_ARGS(&render_target->m_depth_stencil_buffers[v])
			), "Failed to create commited resource.");
			NAME_D3D12RESOURCE(render_target->m_depth_stencil_buffers[v])

			NAME_D3D12RESOURCE(render_target->m_depth_stencil_resource_heap->m_native[v]);
			n_device->CreateDepthStencilView(render_target->m_depth_stencil_buffers[v], &depthStencilDesc, d3d12::GetCPUHandle(render_target->m_depth_stencil_resource_heap, v).m_native);
		}
	}

	void CreateSRVFromDSV(RenderTarget* render_target, DescHeapCPUHandle& handle, unsigned int frame_idx)
	{
		decltype(Device::m_native) n_device;
		render_target->m_render_targets[0]->GetDevice(IID_PPV_ARGS(&n_device));

		unsigned int increment_size = n_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Texture2D.MipLevels = 1;

		n_device->CreateShaderResourceView(render_target->m_depth_stencil_buffers[internal::SaveFrameIdx(frame_idx, render_target)], &srv_desc, handle.m_native);
		Offset(handle, 1, increment_size);
	}

	void CreateSRVFromRTV(RenderTarget* render_target, DescHeapCPUHandle& handle, unsigned int num, Format formats[8], unsigned int frame_idx)
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

			n_device->CreateShaderResourceView(render_target->m_render_targets[i + (render_target->m_num_render_targets * internal::SaveFrameIdx(frame_idx, render_target))], &srv_desc, handle.m_native);
			Offset(handle, 1, increment_size);
		}
	}

	void CreateSRVFromSpecificRTV(RenderTarget* render_target, DescHeapCPUHandle& handle, unsigned int id, Format format, unsigned int frame_idx)
	{
		decltype(Device::m_native) n_device;
		render_target->m_render_targets[0]->GetDevice(IID_PPV_ARGS(&n_device));

		unsigned int increment_size = n_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.Format = (DXGI_FORMAT)format;
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Texture2D.MipLevels = 1;

		n_device->CreateShaderResourceView(render_target->m_render_targets[id + (render_target->m_num_render_targets * internal::SaveFrameIdx(frame_idx, render_target))], &srv_desc, handle.m_native);
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
		for (auto& r : render_target->m_depth_stencil_buffers)
		{
			SAFE_RELEASE(r);
		}
		Destroy(render_target->m_depth_stencil_resource_heap);
	}

	void DestroyRenderTargetViews(RenderTarget* render_target)
	{
		Destroy(render_target->m_rtv_descriptor_heap);
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

} /* wr::d3d12 */
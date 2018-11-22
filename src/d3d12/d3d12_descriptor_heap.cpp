#include "d3d12_functions.hpp"

#include "../util/log.hpp"
#include "d3d12_defines.hpp"
#include "d3dx12.hpp"

namespace wr::d3d12
{
	
	DescriptorHeap* CreateDescriptorHeap(Device* device, desc::DescriptorHeapDesc const & descriptor)
	{
		auto heap = new DescriptorHeap();
		const auto n_device = device->m_native;

		heap->m_create_info = descriptor;
		heap->m_increment_size = n_device->GetDescriptorHandleIncrementSize(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(descriptor.m_type));

		D3D12_DESCRIPTOR_HEAP_DESC heap_desc;
		heap_desc.NumDescriptors = descriptor.m_num_descriptors;
		heap_desc.Type = (D3D12_DESCRIPTOR_HEAP_TYPE)descriptor.m_type;
		heap_desc.Flags = descriptor.m_shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heap_desc.NodeMask = 0;
		HRESULT hr = n_device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap->m_native));

		return heap;
	}

	DescHeapGPUHandle GetGPUHandle(DescriptorHeap* desc_heap, unsigned int index)
	{
		DescHeapGPUHandle retval;;

		retval.m_native = desc_heap->m_native->GetGPUDescriptorHandleForHeapStart();

		if (index > 0)
		{
			Offset(retval, index, desc_heap->m_increment_size);
		}

		return retval;
	}

	DescHeapCPUHandle GetCPUHandle(DescriptorHeap* desc_heap, unsigned int index)
	{
		DescHeapCPUHandle retval;

		retval.m_native = desc_heap->m_native->GetCPUDescriptorHandleForHeapStart();

		if (index > 0)
		{
			Offset(retval, index, desc_heap->m_increment_size);
		}

		return retval;
	}

	void Offset(DescHeapGPUHandle& handle, unsigned int index, unsigned int increment_size)
	{
		handle.m_native.ptr += index * increment_size;
	}

	void Offset(DescHeapCPUHandle& handle, unsigned int index, unsigned int increment_size)
	{
		handle.m_native.ptr += index * increment_size;
	}

	void Destroy(DescriptorHeap* desc_heap)
	{
		SAFE_RELEASE(desc_heap->m_native);
		delete desc_heap;
	}

} /* wr::d3d12 */

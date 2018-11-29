#include "d3d12_functions.hpp"

namespace wr::d3d12
{

	void CreateSRVFromStructuredBuffer(HeapResource* structured_buffer, DescHeapCPUHandle& handle, unsigned int id)
	{
		auto& resources = structured_buffer->m_heap_bsbo->m_resources[structured_buffer->m_heap_vector_location];
		auto& resource = resources.second[id];

		decltype(Device::m_native) n_device;
		resource->GetDevice(IID_PPV_ARGS(&n_device));

		unsigned int increment_size = n_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srv_desc.Buffer.FirstElement = 0;
		srv_desc.Buffer.NumElements = structured_buffer->m_unaligned_size / structured_buffer->m_stride;
		srv_desc.Buffer.StructureByteStride = structured_buffer->m_stride;
		srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		n_device->CreateShaderResourceView(resource, &srv_desc, handle.m_native);
		Offset(handle, 1, increment_size);
	}

}
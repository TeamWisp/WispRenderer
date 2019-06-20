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
		srv_desc.Buffer.NumElements = static_cast<UINT>(structured_buffer->m_unaligned_size / structured_buffer->m_stride);
		srv_desc.Buffer.StructureByteStride = static_cast<UINT>(structured_buffer->m_stride);
		srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		n_device->CreateShaderResourceView(resource, &srv_desc, handle.m_native);
		Offset(handle, 1, increment_size);
	}

}
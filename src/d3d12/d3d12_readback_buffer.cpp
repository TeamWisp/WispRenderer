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

#include "d3d12_defines.hpp"
#include "d3dx12.hpp"
#include "../util/log.hpp"

namespace wr::d3d12
{
	ReadbackBufferResource* wr::d3d12::CreateReadbackBuffer(Device* device, std::uint32_t aligned_buffer_size)
	{
		auto native_device = device->m_native;

		auto* readbackBuffer = new ReadbackBufferResource();

		CD3DX12_HEAP_PROPERTIES heap_properties_readback = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
		CD3DX12_RESOURCE_DESC buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(aligned_buffer_size);

		HRESULT res = native_device->CreateCommittedResource(
			&heap_properties_readback,
			D3D12_HEAP_FLAG_NONE,
			&buffer_desc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&readbackBuffer->m_resource));

		if (FAILED(res))
		{
			LOGC("Error: Could not create a readback buffer!");
		}

		return readbackBuffer;
	}

	void* MapReadbackBuffer(ReadbackBufferResource* const readback_buffer, std::uint32_t buffer_size)
	{
		if (!readback_buffer)
			return nullptr;

		void* memory = nullptr;
		CD3DX12_RANGE buffer_range = CD3DX12_RANGE(0, buffer_size);

		readback_buffer->m_resource->Map(0, &buffer_range, &memory);

		return memory;
	}

	void UnmapReadbackBuffer(ReadbackBufferResource* const readback_buffer)
	{
		if (!readback_buffer)
			return;

		CD3DX12_RANGE buffer_range = CD3DX12_RANGE(0, 0);
		readback_buffer->m_resource->Unmap(0, &buffer_range);
	}

	void SetName(ReadbackBufferResource* readback_buffer, std::wstring name)
	{
		readback_buffer->m_resource->SetName(name.c_str());
	}

	void Destroy(ReadbackBufferResource* readback_buffer)
	{
		SAFE_RELEASE(readback_buffer->m_resource);
		delete readback_buffer;
	}

} /* wr::d3d12 */

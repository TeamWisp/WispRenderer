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

		HRESULT res = native_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(aligned_buffer_size),
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
		readback_buffer->m_resource->Map(0, &CD3DX12_RANGE(0, buffer_size), &memory);
		return memory;
	}

	void UnmapReadbackBuffer(ReadbackBufferResource* const readback_buffer)
	{
		if (!readback_buffer)
			return;

		readback_buffer->m_resource->Unmap(0, &CD3DX12_RANGE(0, 0));
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

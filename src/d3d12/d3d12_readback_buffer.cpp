#include "d3d12_functions.hpp"

#include "d3d12_defines.hpp"
#include "d3dx12.hpp"
#include "../util/log.hpp"

namespace wr::d3d12
{
	ReadbackBufferResource* wr::d3d12::CreateReadbackBuffer(Device* device, desc::ReadbackDesc* description)
	{
		auto native_device = device->m_native;

		auto* readbackBuffer = new ReadbackBufferResource();

		std::uint32_t buffer_size_aligned_to_256 = SizeAlignTwoPower(description->m_buffer_width * description->m_bytes_per_pixel, 256) * description->m_buffer_height;

		CD3DX12_HEAP_PROPERTIES heap_properties_readback = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
		CD3DX12_RESOURCE_DESC buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(buffer_size_aligned_to_256);

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

	void* MapReadbackBuffer(ReadbackBufferResource* const readback_buffer, std::uint64_t buffer_size)
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
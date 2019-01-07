#include "d3d12_functions.hpp"

#include "../util/log.hpp"
#include "d3d12_defines.hpp"
#include "d3dx12.hpp"

namespace wr::d3d12
{
	ReadbackBuffer* wr::d3d12::CreateReadbackBuffer(Device* device, desc::ReadbackDesc* description)
	{
		CD3DX12_HEAP_PROPERTIES readbackHeapProperties(D3D12_HEAP_TYPE_READBACK);

		auto native_device = device->m_native;

		auto* readbackBuffer = new ReadbackBuffer();

		HRESULT res = native_device->CreateCommittedResource(
			&readbackHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(description->m_buffer_size),
			static_cast<D3D12_RESOURCE_STATES>(description->m_initial_state),
			nullptr,
			IID_PPV_ARGS(&readbackBuffer->m_resource));

		if (FAILED(res))
		{
			LOGC("Error: Couldn't create readback buffer");
		}

		return nullptr;
	}

	void SetName(ReadbackBuffer* readback_buffer, std::wstring name)
	{
		readback_buffer->m_resource->SetName(name.c_str());
	}

	void Destroy(ReadbackBuffer* readback_buffer)
	{
		SAFE_RELEASE(readback_buffer->m_resource);
		delete readback_buffer;
	}

} /* wr::d3d12 */
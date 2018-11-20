#include "d3d12_functions.hpp"

#include "d3d12_defines.hpp"

namespace wr::d3d12
{

	StagingBuffer* CreateStagingBuffer(Device* device, void* data, uint64_t size, uint64_t stride, ResourceState resource_state)
	{
		auto buffer = new StagingBuffer();

		buffer->m_target_resource_state = resource_state;
		buffer->m_size = size;
		buffer->m_stride_in_bytes = stride;

		device->m_native->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(size),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&buffer->m_buffer));
		NAME_D3D12RESOURCE(buffer->m_buffer)

		device->m_native->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(size),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&buffer->m_staging));
		NAME_D3D12RESOURCE(buffer->m_staging);

		buffer->m_data = malloc(size);
		memcpy(buffer->m_data, data, size);

		return buffer;
	}

	void StageBuffer(StagingBuffer* buffer, CommandList* cmd_list)
	{
		// store vertex buffer in upload heap
		D3D12_SUBRESOURCE_DATA vertex_data = {};
		vertex_data.pData = buffer->m_data;
		vertex_data.RowPitch = buffer->m_size;
		vertex_data.SlicePitch = buffer->m_size;

		UpdateSubresources(cmd_list->m_native, buffer->m_buffer, buffer->m_staging, 0, 0, 1, &vertex_data);

		// transition the vertex buffer data from copy destination state to vertex buffer state
		cmd_list->m_native->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(buffer->m_buffer, D3D12_RESOURCE_STATE_COPY_DEST, (D3D12_RESOURCE_STATES)buffer->m_target_resource_state));

		buffer->m_gpu_address = buffer->m_buffer->GetGPUVirtualAddress();
	}

	void FreeStagingBuffer(StagingBuffer* buffer)
	{
		SAFE_RELEASE(buffer->m_staging);
	}

	void Destroy(StagingBuffer* buffer)
	{
		SAFE_RELEASE(buffer->m_staging);
		SAFE_RELEASE(buffer->m_buffer);
		delete buffer;
	}

} /* wr::d3d12 */
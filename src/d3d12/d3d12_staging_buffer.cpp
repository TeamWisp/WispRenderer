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
		buffer->m_is_staged = false;

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

		if (data != nullptr) 
		{
			memcpy(buffer->m_data, data, size);
		}

		CD3DX12_RANGE read_range(0, 0);

		buffer->m_staging->Map(0, &read_range, reinterpret_cast<void**>(&(buffer->m_cpu_address)));

		return buffer;
	}

	void UpdateStagingBuffer(StagingBuffer* buffer, void * data, std::uint64_t size, std::uint64_t offset)
	{
		memcpy(static_cast<std::uint8_t*>(buffer->m_data) + offset, data, size);
	}

	void StageBuffer(StagingBuffer* buffer, CommandList* cmd_list)
	{

		if (buffer->m_is_staged) return;

		// store vertex buffer in upload heap
		D3D12_SUBRESOURCE_DATA vertex_data = {};
		vertex_data.pData = buffer->m_data;
		vertex_data.RowPitch = buffer->m_size;
		vertex_data.SlicePitch = buffer->m_size;

		UpdateSubresources(cmd_list->m_native, buffer->m_buffer, buffer->m_staging, 0, 0, 1, &vertex_data);

		// transition the vertex buffer data from copy destination state to vertex buffer state
		cmd_list->m_native->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(buffer->m_buffer, D3D12_RESOURCE_STATE_COPY_DEST, (D3D12_RESOURCE_STATES)buffer->m_target_resource_state));

		buffer->m_gpu_address = buffer->m_buffer->GetGPUVirtualAddress();
		buffer->m_is_staged = true;
	}

	void StageBufferRegion(StagingBuffer * buffer, std::uint64_t size, std::uint64_t offset, CommandList * cmd_list)
	{
		if (buffer->m_is_staged)
		{
			cmd_list->m_native->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(buffer->m_buffer, (D3D12_RESOURCE_STATES)buffer->m_target_resource_state, D3D12_RESOURCE_STATE_COPY_DEST));
		}

		memcpy(buffer->m_cpu_address + offset, static_cast<std::uint8_t*>(buffer->m_data) + offset, size);

		cmd_list->m_native->CopyBufferRegion(buffer->m_buffer, offset, buffer->m_staging, offset, size);

		// transition the vertex buffer data from copy destination state to vertex buffer state
		cmd_list->m_native->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(buffer->m_buffer, D3D12_RESOURCE_STATE_COPY_DEST, (D3D12_RESOURCE_STATES)buffer->m_target_resource_state));

		buffer->m_gpu_address = buffer->m_buffer->GetGPUVirtualAddress();
		buffer->m_is_staged = true;
	}

	void FreeStagingBuffer(StagingBuffer* buffer)
	{
		SAFE_RELEASE(buffer->m_staging);
		buffer->m_is_staged = false;
	}

	void Evict(StagingBuffer * buffer)
	{
		decltype(Device::m_native) n_device;
		buffer->m_staging->GetDevice(IID_PPV_ARGS(&n_device));

		std::array<ID3D12Pageable*, 2> objects{ buffer->m_staging, buffer->m_buffer };
		n_device->Evict(objects.size(), objects.data());
	}

	void MakeResident(StagingBuffer * buffer)
	{
		decltype(Device::m_native) n_device;
		buffer->m_staging->GetDevice(IID_PPV_ARGS(&n_device));

		std::array<ID3D12Pageable*, 2> objects{ buffer->m_staging, buffer->m_buffer };
		n_device->MakeResident(objects.size(), objects.data());
	}

	void Destroy(StagingBuffer* buffer)
	{
		SAFE_RELEASE(buffer->m_staging);
		SAFE_RELEASE(buffer->m_buffer);
		delete buffer;
	}

} /* wr::d3d12 */
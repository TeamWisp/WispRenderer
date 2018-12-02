#include "d3d12_functions.hpp"

#include "../util/log.hpp"
#include "d3d12_defines.hpp"

namespace wr::d3d12
{

	IndirectCommandBuffer* CreateIndirectCommandBuffer(Device* device, std::size_t max_num_buffers, std::size_t command_size)
	{
		auto buffer = new IndirectCommandBuffer();

		buffer->m_max_num_buffers = max_num_buffers;
		buffer->m_num_buffers = 0;
		buffer->m_command_size = command_size;

		TRY(device->m_native->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(max_num_buffers* command_size),
			D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
			nullptr,
			IID_PPV_ARGS(&buffer->m_native)));

		TRY(device->m_native->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(max_num_buffers* command_size),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&buffer->m_native_upload)));

		return buffer;
	}

	void StageBuffer(CommandList* cmd_list, IndirectCommandBuffer* buffer, UINT8* data, std::size_t num_commands)
	{
		D3D12_SUBRESOURCE_DATA cmd_data = {};
		cmd_data.pData = data;
		cmd_data.RowPitch = num_commands * buffer->m_command_size;
		cmd_data.SlicePitch = cmd_data.RowPitch;

		buffer->m_num_buffers = num_commands;

		UpdateSubresources<1>(cmd_list->m_native, buffer->m_native, buffer->m_native_upload, 0, 0, 1, &cmd_data);
	}

} /* wr::d3d12 */
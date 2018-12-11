#include "d3d12_functions.hpp"

#include "../util/log.hpp"
#include "d3d12_defines.hpp"

namespace wr::d3d12
{

	IndirectCommandBuffer* CreateIndirectCommandBuffer(Device* device, std::size_t max_commands, std::size_t command_size, uint32_t versions)
	{
		auto buffer = new IndirectCommandBuffer();

		buffer->m_num_max_commands = max_commands;
		buffer->m_num_commands = 0;
		buffer->m_command_size = command_size;

		buffer->m_native.resize(versions);
		buffer->m_native_upload.resize(versions);

		for (uint32_t i = 0; i < versions; ++i)
		{
			TRY(device->m_native->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(max_commands * command_size),
				D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
				nullptr,
				IID_PPV_ARGS(buffer->m_native.data() + i)));

			TRY(device->m_native->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(max_commands * command_size),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(buffer->m_native_upload.data() + i)));

		}

		return buffer;
	}

	void SetName(IndirectCommandBuffer* buffer, std::wstring name)
	{
		for (uint32_t i = 0, j = (uint32_t)buffer->m_native.size(); i < j; ++i)
		{
			buffer->m_native[i]->SetName((name + L" #" + std::to_wstring(i)).c_str());
			buffer->m_native_upload[i]->SetName((name + L" #" + std::to_wstring(i) + L" Upload Buffer").c_str());
		}
	}

	void StageBuffer(CommandList* cmd_list, IndirectCommandBuffer* buffer, void* data, std::size_t num_commands, uint32_t frame_idx)
	{
		D3D12_SUBRESOURCE_DATA cmd_data = {};
		cmd_data.pData = reinterpret_cast<UINT8*>(data);
		cmd_data.RowPitch = num_commands * buffer->m_command_size;
		cmd_data.SlicePitch = cmd_data.RowPitch;

		buffer->m_num_commands = num_commands;

		UpdateSubresources<1>(cmd_list->m_native, buffer->m_native[frame_idx], buffer->m_native_upload[frame_idx], 0, 0, 1, &cmd_data);
	}

} /* wr::d3d12 */
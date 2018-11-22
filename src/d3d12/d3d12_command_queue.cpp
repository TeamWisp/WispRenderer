#include "d3d12_functions.hpp"

#include "../util/log.hpp"
#include "d3d12_defines.hpp"

namespace wr::d3d12
{

	CommandQueue* CreateCommandQueue(Device* device, CmdListType type)
	{
		auto cmd_queue = new CommandQueue();
		const auto n_device = device->m_native;

		D3D12_COMMAND_QUEUE_DESC cmd_queue_desc = {};
		cmd_queue_desc.Flags = settings::enable_gpu_timeout ? D3D12_COMMAND_QUEUE_FLAG_NONE : D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT;
		cmd_queue_desc.Type = static_cast<D3D12_COMMAND_LIST_TYPE>(type);

		TRY_M(n_device->CreateCommandQueue(&cmd_queue_desc, IID_PPV_ARGS(&cmd_queue->m_native)),
			"Failed to create DX12 command queue.");
		NAME_D3D12RESOURCE(cmd_queue->m_native);

		return cmd_queue;
	}

	void Execute(CommandQueue* cmd_queue, std::vector<CommandList*> const & cmd_lists, Fence* fence)
	{
		std::vector<ID3D12CommandList*> native_lists;
		native_lists.resize(cmd_lists.size());
		for (auto i = 0; i < native_lists.size(); i++)
		{
			native_lists[i] = cmd_lists[i]->m_native;
		}

		cmd_queue->m_native->ExecuteCommandLists(native_lists.size(), native_lists.data());

		Signal(fence, cmd_queue);
	}

	void Destroy(CommandQueue* cmd_queue)
	{
		if (cmd_queue == nullptr) {
			return;
		}

		SAFE_RELEASE(cmd_queue->m_native);
		delete cmd_queue;
	}	

} /* wr::d3d12 */
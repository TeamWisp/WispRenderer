#include "d3d12_functions.hpp"

namespace d3d12
{

	/* === Device ===*/
	Device* CreateDevice()
	{
		return new Device();
	}

	void Destroy(Device* device)
	{
		device->m_adapter->Release();
		device->m_native->Release();
		device->m_dxgi_factory->Release();
	}

	/* === CommandQueue ===*/
	CommandQueue* CreateCommandQueue(Device* device, CmdListType type)
	{
		return new CommandQueue();
	}

	void Execute(CommandQueue* cmd_queue, std::vector<VCommandList> const & cmd_lists, VFence* fence)
	{

	}

	void Destroy(CommandQueue* cmd_queue)
	{
		cmd_queue->m_native->Release();
	}

} /* d3d12 */
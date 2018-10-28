#include "d3d12_functions.hpp"

#include "d3d12_defines.hpp"

namespace d3d12
{

	/* === Device ===*/
	Device* CreateDevice()
	{
		return new Device();
	}

	void Destroy(Device* device)
	{
		SAFE_RELEASE(device->m_adapter);
		SAFE_RELEASE(device->m_native);
		SAFE_RELEASE(device->m_dxgi_factory);
#ifdef _DEBUG
		SAFE_RELEASE(device->m_debug_controller);
#endif
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
		SAFE_RELEASE(cmd_queue->m_native);
	}	

} /* d3d12 */
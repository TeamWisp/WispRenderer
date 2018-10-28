#pragma once

#include <memory>

#include "d3d12_structs.hpp"

namespace d3d12
{

	// Device
	Device* CreateDevice();
	void Destroy(Device* device);

	// CommandQueue
	CommandQueue* CreateCommandQueue(Device* device, CmdListType type);
	void Execute(CommandQueue* cmd_queue, std::vector<VCommandList> const & cmd_lists, VFence* fence);
	void Destroy(CommandQueue* cmd_queue);

} /* d3d12 */
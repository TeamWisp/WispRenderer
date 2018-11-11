#include "d3d12_functions.hpp"

#include "../util/log.hpp"
#include "d3d12_defines.hpp"

namespace d3d12
{

	Fence* CreateFence(Device* device)
	{
		auto fence = new Fence();
		auto n_device = device->m_native;

		// create the fences
		TRY_M(n_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence->m_native)),
			"Failed to create fence");
		fence->m_native->SetName(L"SimpleFence");

		// create a handle to a fence event
		fence->m_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (fence->m_fence_event == nullptr)
		{
			LOGC("Failed to create fence event");
		}

		return fence;
	}

	void Signal(Fence* fence, CommandQueue* cmd_queue)
	{
		TRY_M(cmd_queue->m_native->Signal(fence->m_native, fence->m_fence_value),
			"Failed to set fence signal");
	}

	void WaitFor(Fence* fence)
	{
		if (fence->m_native->GetCompletedValue() < fence->m_fence_value)
		{
			// we have the fence create an event which is signaled once the fence's current value is "fenceValue"
			TRY_M(fence->m_native->SetEventOnCompletion(fence->m_fence_value, fence->m_fence_event),
				"Failed to set fence event.");

			WaitForSingleObject(fence->m_fence_event, INFINITE);
		}

		// increment fenceValue for next frame (or usage)
		fence->m_fence_value++;
	}

	void Destroy(Fence* fence)
	{
		SAFE_RELEASE(fence->m_native)
		CloseHandle(fence->m_fence_event);
		delete fence;
	}

} /* d3d12 */
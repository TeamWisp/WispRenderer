/*!
 * Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "d3d12_functions.hpp"

#include "../util/log.hpp"
#include "d3d12_defines.hpp"

namespace wr::d3d12
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

	void SetName(Fence * fence, std::wstring name)
	{
		fence->m_native->SetName(name.c_str());
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

} /* wr::d3d12 */
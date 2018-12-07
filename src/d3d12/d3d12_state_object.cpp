#include "d3d12_functions.hpp"

#include "d3d12_defines.hpp"
#include "d3dx12.hpp"

namespace wr::d3d12
{

	[[nodiscard]] StateObject* CreateStateObject(Device* device, CD3DX12_STATE_OBJECT_DESC desc)
	{
		auto state_object = new StateObject();

		// Create the state object.
		if (GetRaytracingType(device) == RaytracingType::NATIVE)
		{
			TRY_M(device->m_native->CreateStateObject(desc, IID_PPV_ARGS(&state_object->m_native)),
				"Couldn't create DirectX Raytracing state object.");
		}
		else if(GetRaytracingType(device) == RaytracingType::FALLBACK)
		{
			TRY_M(device->m_fallback_native->CreateStateObject(desc, IID_PPV_ARGS(&state_object->m_fallback_native)),
				"Couldn't create DirectX Fallback Raytracing state object.");
		}

		return state_object;
	}

	void Destroy(StateObject* obj)
	{
		SAFE_RELEASE(obj->m_native);
		SAFE_RELEASE(obj->m_properties);
		SAFE_RELEASE(obj->m_fallback_native);
	}

} /* wr::d3d12 */
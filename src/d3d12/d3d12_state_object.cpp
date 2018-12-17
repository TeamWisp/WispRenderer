#include "d3d12_functions.hpp"

#include "d3d12_defines.hpp"
#include "d3dx12.hpp"

#include <wrl/client.h>

namespace wr::d3d12
{

	[[nodiscard]] StateObject* CreateStateObject(Device* device, CD3DX12_STATE_OBJECT_DESC desc)
	{
		auto state_object = new StateObject();

		state_object->m_desc = desc;
		state_object->m_device = device;

		// Create the state object.
		if (GetRaytracingType(device) == RaytracingType::NATIVE)
		{
			HRESULT hr;
			TRY_M(hr = device->m_native->CreateStateObject(desc, IID_PPV_ARGS(&state_object->m_native)),
				"Couldn't create DirectX Raytracing state object.");

			// Shitty code because I don't know what As does.
			Microsoft::WRL::ComPtr<ID3D12StateObject> temp_state_object(state_object->m_native);
			Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> temp_properties;

			TRY_M(temp_state_object.As(&temp_properties), "Failed to do ComPtr.As");
			state_object->m_properties = temp_properties.Get();

			temp_state_object.Detach();
			temp_properties.Detach();
		}
		else if(GetRaytracingType(device) == RaytracingType::FALLBACK)
		{
			TRY_M(device->m_fallback_native->CreateStateObject(desc, IID_PPV_ARGS(&state_object->m_fallback_native)),
				"Couldn't create DirectX Fallback Raytracing state object.");
		}

		return state_object;
	}

	void RecreateStateObject(StateObject* state_object)
	{
		*state_object = *CreateStateObject(state_object->m_device, state_object->m_desc);
	}

	std::uint64_t GetShaderIdentifierSize(Device* device, StateObject* obj)
	{
		// Get shader identifiers.
		std::uint64_t retval = 0;
		if (GetRaytracingType(device) == RaytracingType::NATIVE)
		{
			retval = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
		}
		else if (GetRaytracingType(device) == RaytracingType::FALLBACK) // DirectX Raytracing
		{
			retval = device->m_fallback_native->GetShaderIdentifierSize();
		}

		return retval;
	}

	void SetGlobalRootSignature(StateObject* state_object, RootSignature* global_root_signature)
	{
		state_object->m_global_root_signature = global_root_signature;
	}

	[[nodiscard]] void* GetShaderIdentifier(Device* device, StateObject* obj, std::string const & name)
	{	
		std::wstring wname(name.begin(), name.end());

		// Reusable lambda
		auto GetShaderIdentifiers = [&](auto* stateObjectProperties)
		{
			return stateObjectProperties->GetShaderIdentifier(wname.c_str());
		};

		// Get shader identifiers.
		std::uint64_t retval;
		if (GetRaytracingType(device) == RaytracingType::NATIVE)
		{
			return GetShaderIdentifiers(obj->m_properties);
		}
		else if (GetRaytracingType(device) == RaytracingType::FALLBACK) // DirectX Raytracing
		{
			return GetShaderIdentifiers(obj->m_fallback_native);
		}

		LOGC("GetShaderIdentifier Called but raytracing isn't enabled!");

		return nullptr;
	}

	void SetName(StateObject * obj, std::wstring name)
	{
		obj->m_native->SetName(name.c_str());
	}

	void Destroy(StateObject* obj)
	{
		SAFE_RELEASE(obj->m_native);
		SAFE_RELEASE(obj->m_properties);
		SAFE_RELEASE(obj->m_fallback_native);
	}

} /* wr::d3d12 */

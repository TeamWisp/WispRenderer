// Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "d3d12_functions.hpp"

#include "d3d12_defines.hpp"
#include "d3dx12.hpp"

#include <wrl/client.h>

namespace wr::d3d12
{

	[[nodiscard]] StateObject* CreateStateObject(Device* device, desc::StateObjectDesc desc)
	{
		auto state_object = new StateObject();

		state_object->m_desc = desc;
		state_object->m_device = device;

		// Create CD3DX12_STATE_OBJECT_DESC
		CD3DX12_STATE_OBJECT_DESC n_desc = { D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

		// Shader Library
		{
			D3D12_SHADER_BYTECODE bytecode = {};
			bytecode.BytecodeLength = desc.m_library->m_native->GetBufferSize();
			bytecode.pShaderBytecode = desc.m_library->m_native->GetBufferPointer();
			auto lib = n_desc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
			for (auto exp : desc.m_library_exports)
			{
				lib->DefineExport(exp.c_str());
			}
			lib->SetDXILLibrary(&bytecode);
		}

		// Shader Config
		{
			auto shader_config = n_desc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
			shader_config->Config(desc.max_payload_size, desc.max_attributes_size);
		}

		// Hitgroup
		{
			for (auto def : desc.m_hit_groups)
			{
				auto hitGroup = n_desc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
				hitGroup->SetClosestHitShaderImport(def.second.c_str());
				hitGroup->SetHitGroupExport(def.first.c_str());
				hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
			}
		}

		// Global Root Signature
		if (auto rs = desc.global_root_signature.value_or(nullptr); desc.global_root_signature.has_value())
		{
			auto global_rs = n_desc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
			global_rs->SetRootSignature(rs->m_native);
			state_object->m_global_root_signature = rs;
		}

		// Local Root Signatures
		if (desc.local_root_signatures.has_value())
		{
			for (auto& rs : desc.local_root_signatures.value())
			{
				auto local_rs = n_desc.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
				local_rs->SetRootSignature(rs->m_native);
				// Define explicit shader association for the local root signature.
				{
					//auto rootSignatureAssociation = desc.desc.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
					//rootSignatureAssociation->SetSubobjectToAssociate(*local_rs);
					//rootSignatureAssociation->AddExport(L"MyHitGroup");
				}
			}
		}

		// Pipeline Config
		{
			auto pipeline_config = n_desc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
			pipeline_config->Config(desc.max_recursion_depth);
		}

		// Create the state object.
		if (GetRaytracingType(device) == RaytracingType::NATIVE)
		{
			HRESULT hr;
			TRY_M(hr = device->m_native->CreateStateObject(n_desc, IID_PPV_ARGS(&state_object->m_native)),
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
			TRY_M(device->m_fallback_native->CreateStateObject(n_desc, IID_PPV_ARGS(&state_object->m_fallback_native)),
				"Couldn't create DirectX Fallback Raytracing state object.");
		}

		//n_desc.DeleteHelpers();

		return state_object;
	}

	void RecreateStateObject(StateObject* state_object)
	{
		*state_object = *CreateStateObject(state_object->m_device, state_object->m_desc);
	}

	std::uint64_t GetShaderIdentifierSize(Device* device)
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

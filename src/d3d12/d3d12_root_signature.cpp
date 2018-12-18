#include "d3d12_functions.hpp"

#include "d3d12_defines.hpp"

namespace wr::d3d12
{

	RootSignature* CreateRootSignature(desc::RootSignatureDesc create_info)
	{
		auto root_signature = new RootSignature();

		root_signature->m_create_info = create_info;

		return root_signature;
	}

	void SetName(RootSignature * root_signature, std::wstring name)
	{
		root_signature->m_native->SetName(name.c_str());
	}

	void FinalizeRootSignature(RootSignature* root_signature, Device* device)
	{
		auto n_device = device->m_native;
		auto n_fb_device = device->m_fallback_native;
		auto create_info = root_signature->m_create_info;

		// Check capabilities TODO: Actually use capabilities.
		D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data = {};
		feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(device->m_native->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature_data, sizeof(feature_data))))
		{
			feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		auto num_samplers = create_info.m_samplers.size();
		std::vector<D3D12_STATIC_SAMPLER_DESC> samplers(num_samplers);
		for (auto i = 0; i < num_samplers; i++)
		{
			auto sampler_info = create_info.m_samplers[i];

			samplers[0].Filter = (D3D12_FILTER)sampler_info.m_filter;
			samplers[0].AddressU = (D3D12_TEXTURE_ADDRESS_MODE)sampler_info.m_address_mode;
			samplers[0].AddressV = (D3D12_TEXTURE_ADDRESS_MODE)sampler_info.m_address_mode;
			samplers[0].AddressW = (D3D12_TEXTURE_ADDRESS_MODE)sampler_info.m_address_mode;
			samplers[0].MipLODBias = 0;
			samplers[0].MaxAnisotropy = 0;
			samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
			samplers[0].BorderColor = (D3D12_STATIC_BORDER_COLOR)sampler_info.m_border_color;
			samplers[0].MinLOD = 0.0f;
			samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
			samplers[0].ShaderRegister = i;
			samplers[0].RegisterSpace = 0;
			samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // TODO: very inneficient. plz fix
		}

		D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		if (create_info.m_rt_local)
		{
			flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
		}

		CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc;
		root_signature_desc.Init(create_info.m_parameters.size(),
			create_info.m_parameters.data(),
			num_samplers,
			samplers.data(),
			flags);

		if (create_info.m_rtx && GetRaytracingType(device) == RaytracingType::FALLBACK)
		{
			// Fallback
			ID3DBlob* signature;
			ID3DBlob* error = nullptr;
			HRESULT hr = n_fb_device->D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error); //TODO: FIX error parameter
			if (FAILED(hr))
			{
				LOGC("Failed to serialize root signature. Error: \n {}", (char*)error->GetBufferPointer());
			}

			TRY_M(n_fb_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&root_signature->m_native)),
				"Failed to create root signature");
		}
		else
		{
			ID3DBlob* signature;
			ID3DBlob* error = nullptr;
			HRESULT hr = D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error); //TODO: FIX error parameter
			if (FAILED(hr))
			{
				char * err = (char*)error->GetBufferPointer();
				LOGC("Failed to serialize root signature. Error: \n {}", err);
			}

			TRY_M(n_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&root_signature->m_native)),
				"Failed to create root signature");
		}
		NAME_D3D12RESOURCE(root_signature->m_native);
	}

	void Destroy(RootSignature* root_signature)
	{
		SAFE_RELEASE(root_signature->m_native);
		delete root_signature;
	}

} /* wr::d3d12 */
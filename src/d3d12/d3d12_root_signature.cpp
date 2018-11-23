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

	void FinalizeRootSignature(RootSignature* root_signature, Device* device)
	{
		auto n_device = device->m_native;

		// Check capabilities TODO: Actually use capabilities.
		D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data = {};
		feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(device->m_native->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature_data, sizeof(feature_data))))
		{
			feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		auto num_samplers = root_signature->m_create_info.m_samplers.size();
		std::vector<D3D12_STATIC_SAMPLER_DESC> samplers(num_samplers);
		for (auto i = 0; i < num_samplers; i++)
		{
			auto sampler_info = root_signature->m_create_info.m_samplers[i];

			samplers[i].Filter = (D3D12_FILTER)sampler_info.m_filter;
			samplers[i].AddressU = (D3D12_TEXTURE_ADDRESS_MODE)sampler_info.m_address_mode;
			samplers[i].AddressV = (D3D12_TEXTURE_ADDRESS_MODE)sampler_info.m_address_mode;
			samplers[i].AddressW = (D3D12_TEXTURE_ADDRESS_MODE)sampler_info.m_address_mode;
			samplers[i].MipLODBias = 0;
			samplers[i].MaxAnisotropy = 0;
			samplers[i].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
			samplers[i].BorderColor = (D3D12_STATIC_BORDER_COLOR)sampler_info.m_border_color;
			samplers[i].MinLOD = 0.0f;
			samplers[i].MaxLOD = D3D12_FLOAT32_MAX;
			samplers[i].ShaderRegister = i;
			samplers[i].RegisterSpace = 0;
			samplers[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // TODO: very inneficient. plz fix
		}

		std::vector<CD3DX12_ROOT_PARAMETER> parameters(root_signature->m_create_info.m_parameters.size() + root_signature->m_create_info.m_descriptorRanges.size());

		for (size_t i = 0, j = parameters.size(), k = root_signature->m_create_info.m_parameters.size(); i < j; ++i) {
			if (i < k)
				parameters[i] = root_signature->m_create_info.m_parameters[i];
			else
				parameters[i].InitAsDescriptorTable(1, root_signature->m_create_info.m_descriptorRanges.data() + i - k, D3D12_SHADER_VISIBILITY_ALL); // TODO: very inneficient. plz fix
		}

		CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc;
		root_signature_desc.Init((UINT)parameters.size(),
			parameters.data(),
			num_samplers,
			samplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ID3DBlob* signature;
		ID3DBlob* error = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error); //TODO: FIX error parameter
		if (FAILED(hr))
		{
			LOGC("Failed to serialize root signature. Error: \n {}", (char)error->GetBufferPointer());
			throw "Failed to create a serialized root signature";
		}

		TRY_M(n_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&root_signature->m_native)),
			"Failed to create root signature");
		NAME_D3D12RESOURCE(root_signature->m_native);
	}

	void Destroy(RootSignature* root_signature)
	{
		SAFE_RELEASE(root_signature->m_native);
		delete root_signature;
	}

} /* wr::d3d12 */
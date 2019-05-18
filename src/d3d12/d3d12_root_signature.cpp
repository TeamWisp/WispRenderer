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

		auto num_samplers = static_cast<std::uint32_t>(create_info.m_samplers.size());
		std::vector<D3D12_STATIC_SAMPLER_DESC> samplers(num_samplers);
		for (auto i = 0; i < num_samplers; i++)
		{
			auto sampler_info = create_info.m_samplers[i];

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

		//Create root signature bit masks, used for dynamic gpu descriptor heaps
		auto num_params = create_info.m_parameters.size();

		for (size_t i = 0; i < num_params; ++i)
		{
			auto param = create_info.m_parameters.at(i);

			if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
			{
				auto num_desc_ranges = param.DescriptorTable.NumDescriptorRanges;

				// Set the bit mask depending on the type of descriptor table.
				if (num_desc_ranges > 0)
				{
					switch (param.DescriptorTable.pDescriptorRanges[0].RangeType)
					{
					case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
					case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
					case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
						root_signature->m_descriptor_table_bit_mask |= (1 << i);
						break;
					case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
						root_signature->m_sampler_table_bit_mask |= (1 << i);
						break;
					}
				}

				for (size_t j = 0; j < num_desc_ranges; ++j)
				{
					root_signature->m_num_descriptors_per_table[i] += param.DescriptorTable.pDescriptorRanges[j].NumDescriptors;
				}
			}
		}

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

	void RefinalizeRootSignature(RootSignature* root_signature, Device* device)
	{
		SAFE_RELEASE(root_signature->m_native)
		FinalizeRootSignature(root_signature, device);
	}

	void Destroy(RootSignature* root_signature)
	{
		SAFE_RELEASE(root_signature->m_native);
		delete root_signature;
	}

} /* wr::d3d12 */
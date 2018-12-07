#pragma once

#include "registry.hpp"

#include <vector>

#include "d3d12/d3d12_enums.hpp"
#include "d3d12/d3d12_structs.hpp"
#include "d3d12/d3dx12.hpp"

namespace wr
{
	struct RootSignature
	{

	};

	struct RootSignatureDescription
	{
		std::vector<CD3DX12_ROOT_PARAMETER> m_parameters; // TODO: Write platform independend version.
		std::vector<d3d12::desc::SamplerDesc> m_samplers; // TODO: Move to platform independed location
		bool m_rtx = false;
		bool m_rtx_local = false;
	};

	class RootSignatureRegistry : public internal::Registry<RootSignatureRegistry, RootSignature, RootSignatureDescription>
	{
	public:
		RootSignatureRegistry();
		virtual ~RootSignatureRegistry() = default;

		RegistryHandle Register(RootSignatureDescription description);
	};

} /* wr */
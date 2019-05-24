#pragma once

#include "registry.hpp"

#include <vector>

#include "d3d12/d3d12_enums.hpp"
#include "d3d12/d3d12_structs.hpp"
#include "d3d12/d3dx12.hpp"
#include "util/named_type.hpp"

namespace wr
{
	using RootSignature = void;

	struct RootSignatureDescription
	{
		using Parameters = util::NamedType<std::vector<CD3DX12_ROOT_PARAMETER>>;
		using Samplers = util::NamedType<std::vector<d3d12::desc::SamplerDesc>>;
		using ForRTX = util::NamedType<bool>;
		using RTXLocal = util::NamedType<bool>;
		using Name = util::NamedType<std::wstring>;

		Parameters m_parameters; // TODO: Write platform independend version.
		Samplers m_samplers; // TODO: Move to platform independed location
		ForRTX m_rtx = ForRTX(false);
		RTXLocal m_rtx_local = RTXLocal(false);
		Name name = Name(L"Unknown root signature");
	};

	class RootSignatureRegistry : public internal::Registry<RootSignatureRegistry, RootSignature, RootSignatureDescription>
	{
	public:
		RootSignatureRegistry();
		virtual ~RootSignatureRegistry() = default;

		RegistryHandle Register(RootSignatureDescription description);
	};

} /* wr */

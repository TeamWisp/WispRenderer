#pragma once

#include "registry.hpp"

#include <vector>

#include "d3d12/d3d12_enums.hpp"
#include "d3d12/d3d12_structs.hpp"
#include "d3d12/d3dx12.hpp"
#include "util/named_type.hpp"

namespace wr
{
	struct RootSignature
	{

	};

	struct RootSignatureDescription
	{
		using Parameters = util::NamedType<std::vector<CD3DX12_ROOT_PARAMETER>>;
		using Samplers   = util::NamedType<std::vector<d3d12::desc::SamplerDesc>>;
		using DXR        = util::NamedType<bool>;
		using DXRLocal   = util::NamedType<bool>;
		using Name       = util::NamedType<std::wstring>;

		Parameters m_parameters; // TODO: Write platform independend version.
		Samplers m_samplers; // TODO: Move to platform independed location
		DXR m_rtx = decltype(m_rtx)(false);
		DXRLocal m_rtx_local = decltype(m_rtx_local)(false);
		Name name = decltype(name)(L"unknown root signature");
	};

	class RootSignatureRegistry : public internal::Registry<RootSignatureRegistry, RootSignature, RootSignatureDescription>
	{
	public:
		RootSignatureRegistry();
		virtual ~RootSignatureRegistry() = default;

		RegistryHandle Register(RootSignatureDescription description);
	};

} /* wr */
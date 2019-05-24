#pragma once

#include "registry.hpp"

#include <array>
#include <optional>

#include "vertex.hpp"
#include "d3d12/d3dx12.hpp"
#include "d3d12/d3d12_enums.hpp"
#include "util/named_type.hpp"

namespace wr
{

	using StateObject = void;

	struct StateObjectDescription
	{
		struct LibraryDesc
		{
			RegistryHandle shader_handle;
			std::vector<std::wstring> exports;
			std::vector<std::pair<std::wstring, std::wstring>> m_hit_groups; // first = hit group | second = entry
		};

		using Library = util::NamedType<LibraryDesc>;
		using D3D12StateObjectDesc = util::NamedType<CD3DX12_STATE_OBJECT_DESC>;
		using MaxPayloadSize = util::NamedType<std::uint64_t>;
		using MaxAttributeSize = util::NamedType<std::uint64_t>;
		using MaxRecursionDepth = util::NamedType<std::uint64_t>;
		using GlobalRootSignature = util::NamedType<std::optional<RegistryHandle>>;
		using LocalRootSignatures = util::NamedType<std::optional<std::vector<RegistryHandle>>>;

		D3D12StateObjectDesc desc;
		Library library_desc;

		MaxPayloadSize max_payload_size;
		MaxAttributeSize max_attributes_size;
		MaxRecursionDepth max_recursion_depth;

		GlobalRootSignature global_root_signature;
		LocalRootSignatures local_root_signatures;
	};

	class RTPipelineRegistry : public internal::Registry<RTPipelineRegistry, StateObject, StateObjectDescription>
	{
	public:
		RTPipelineRegistry();
		virtual ~RTPipelineRegistry() = default;

		RegistryHandle Register(StateObjectDescription description)
		{
			auto handle = m_next_handle;

			m_descriptions.insert({ handle, description });

			m_next_handle++;
			return handle;
		}
	};

} /* wr */

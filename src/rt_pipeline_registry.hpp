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

	struct StateObject { };

	struct StateObjectDescription
	{
		struct Library
		{
			RegistryHandle shader_handle;
			std::vector<std::wstring> exports;
		};

		using StateObjectDesc = fluent::NamedType<CD3DX12_STATE_OBJECT_DESC, StateObjectDescription>;
		using LibraryDesc = fluent::NamedType<Library, StateObjectDescription>;
		using MaxPayloadSize = fluent::NamedType<std::uint64_t, StateObjectDescription>;
		using MaxAttributeSize = fluent::NamedType<std::uint64_t, StateObjectDescription>;
		using MaxRecursionDepth = fluent::NamedType<std::uint64_t, StateObjectDescription>;
		using GlobalRootSignature = fluent::NamedType<std::optional<RegistryHandle>, StateObjectDescription>;
		using LocalRootSignatures = fluent::NamedType<std::optional<std::vector<RegistryHandle>>, StateObjectDescription>;

		StateObjectDesc desc;
		LibraryDesc library_desc;

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
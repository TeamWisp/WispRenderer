#pragma once

#include "registry.hpp"

#include <array>
#include <optional>

#include "vertex.hpp"
#include "d3d12/d3dx12.hpp"
#include "d3d12/d3d12_enums.hpp"

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

		CD3DX12_STATE_OBJECT_DESC desc;
		Library library_desc;

		std::uint64_t max_payload_size;
		std::uint64_t max_attributes_size;
		std::uint64_t max_recursion_depth;

		std::optional<RegistryHandle> global_root_signature;
		std::optional<std::vector<RegistryHandle>> local_root_signatures;
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
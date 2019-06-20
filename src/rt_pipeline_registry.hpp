/*!
 * Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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

		CD3DX12_STATE_OBJECT_DESC desc;
		LibraryDesc library_desc;

		std::uint64_t max_payload_size;
		std::uint64_t max_attributes_size;
		std::uint64_t max_recursion_depth;

		std::optional<RegistryHandle> global_root_signature;
		std::vector<RegistryHandle> local_root_signatures;
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

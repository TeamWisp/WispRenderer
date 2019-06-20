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
		std::vector<CD3DX12_ROOT_PARAMETER> m_parameters; // TODO: Write platform independend version.
		std::vector<d3d12::desc::SamplerDesc> m_samplers; // TODO: Move to platform independed location
		bool m_rtx = false;
		bool m_rtx_local = false;
		std::wstring name = L"Unknown root signature";
	};

	class RootSignatureRegistry : public internal::Registry<RootSignatureRegistry, RootSignature, RootSignatureDescription>
	{
	public:
		RootSignatureRegistry();
		virtual ~RootSignatureRegistry() = default;

		RegistryHandle Register(RootSignatureDescription description);
	};

} /* wr */

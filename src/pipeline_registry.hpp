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
#include "util/named_type.hpp"
#include "d3d12/d3d12_enums.hpp"

namespace wr
{

	using Pipeline = void;

	struct PipelineDescription
	{
		std::optional<RegistryHandle> m_vertex_shader_handle;
		std::optional<RegistryHandle> m_pixel_shader_handle;
		std::optional<RegistryHandle> m_compute_shader_handle;
		RegistryHandle m_root_signature_handle;

		Format m_dsv_format;
		std::array<Format, 8> m_rtv_formats;
		unsigned int m_num_rtv_formats;

		PipelineType m_type = PipelineType::GRAPHICS_PIPELINE;
		CullMode m_cull_mode = wr::CullMode::CULL_BACK;
		bool m_depth_enabled = false;
		bool m_counter_clockwise = false;
		TopologyType m_topology_type = wr::TopologyType::TRIANGLE;

		std::vector<D3D12_INPUT_ELEMENT_DESC> m_input_layout = {};
	};

	class PipelineRegistry : public internal::Registry<PipelineRegistry, Pipeline, PipelineDescription>
	{
	public:
		PipelineRegistry();
		virtual ~PipelineRegistry() = default;

		template<typename VT>
		RegistryHandle Register(PipelineDescription description);
	};

	template<typename VT>
	RegistryHandle PipelineRegistry::Register(PipelineDescription description)
	{
		IS_PROPER_VERTEX_CLASS(VT)

		auto handle = m_next_handle;

		description.m_input_layout = VT::GetInputLayout();
		m_descriptions.insert({ handle, description });

		m_next_handle++;
		return handle;
	}

} /* wr */
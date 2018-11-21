#pragma once

#include "registry.hpp"

#include <array>
#include <optional>

#include "vertex.hpp"
#include "d3d12/d3d12_enums.hpp"

namespace wr
{

	struct Pipeline { };

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
		CullMode m_cull_mode = CullMode::CULL_BACK;
		bool m_depth_enabled = false;
		bool m_counter_clockwise = false;
		TopologyType m_topology_type = TopologyType::TRIANGLE;

		std::vector<D3D12_INPUT_ELEMENT_DESC> m_input_layout;
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
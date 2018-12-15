#pragma once

#include "registry.hpp"

#include <array>
#include <optional>

#include "vertex.hpp"
#include "d3d12/d3d12_enums.hpp"
#include "util/named_type.hpp"

namespace wr
{

	struct Pipeline { };

	struct PipelineDescription
	{
		using VertexShader     = util::NamedType<std::optional<RegistryHandle>>;
		using PixelShader      = util::NamedType<std::optional<RegistryHandle>>;
		using ComputeShader    = util::NamedType<std::optional<RegistryHandle>>;
		using RootSignature    = util::NamedType<RegistryHandle>;
		using DSVFormat        = util::NamedType<Format>;
		using RTVFormats       = util::NamedType<std::array<Format, 8>>;
		using NumRTVFormats    = util::NamedType<unsigned int>;
		using Type             = util::NamedType<PipelineType>;
		using CullMode         = util::NamedType<CullMode>;
		using Depth            = util::NamedType<bool>;
		using CounterClockwise = util::NamedType<bool>;
		using TopologyType     = util::NamedType<TopologyType>;
		using InputLayout      = util::NamedType<std::vector<D3D12_INPUT_ELEMENT_DESC>>;

		VertexShader m_vertex_shader_handle;
		PixelShader m_pixel_shader_handle;
		ComputeShader m_compute_shader_handle;
		RootSignature m_root_signature_handle;

		DSVFormat m_dsv_format;
		RTVFormats m_rtv_formats;
		NumRTVFormats m_num_rtv_formats;

		Type m_type = Type(PipelineType::GRAPHICS_PIPELINE);
		CullMode m_cull_mode = CullMode(wr::CullMode::CULL_BACK);
		Depth m_depth_enabled = Depth(false);
		CounterClockwise m_counter_clockwise = CounterClockwise(false);
		TopologyType m_topology_type = TopologyType(wr::TopologyType::TRIANGLE);

		InputLayout m_input_layout = InputLayout({});
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

		description.m_input_layout = PipelineDescription::InputLayout(VT::GetInputLayout());
		m_descriptions.insert({ handle, description });

		m_next_handle++;
		return handle;
	}

} /* wr */
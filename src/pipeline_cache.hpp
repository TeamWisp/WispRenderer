#pragma once

// TODO: Abstract input layout
// TODO: Cache them to disc.
// TODO: Deprecated d3d12::pipelinedesc in favor of the platform independed one.

#include <array>
#include <vector>
#include <d3d12.h>
#include <unordered_map>

#include "vertex.hpp"
#include "d3d12/d3d12_enums.hpp"

namespace wr
{

	using PipelineHandle = std::uint64_t;

	struct PipelineDescription
	{
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

	class PipelineCache
	{
	public:
		PipelineCache();
		virtual ~PipelineCache() = default;

		template<typename VT>
		static PipelineHandle RegisterPipelineDescription(PipelineDescription description);

		virtual void PreparePipelines() = 0;

	protected:
		static std::unordered_map<PipelineHandle, PipelineDescription> m_pipeline_descs;
		static PipelineHandle m_next_handle;

		bool m_loaded;
	};

	template<typename VT>
	static PipelineHandle PipelineCache::RegisterPipelineDescription(PipelineDescription description)
	{
		IS_PROPER_VERTEX_CLASS(VT)

		auto handle = m_next_handle;

		description.m_input_layout = VT::GetInputLayout();
		m_pipeline_descs.insert({ handle, description });

		m_next_handle++;
		return handle;
	}

} /* wr */

namespace wr::ps_registry
{
	static auto default_ps = PipelineCache::RegisterPipelineDescription<Vertex>({
		Format::UNKNOWN,
		{ Format::R8G8B8A8_UNORM },
		1,
		PipelineType::GRAPHICS_PIPELINE,
		CullMode::CULL_BACK,
		false,
		false,
		TopologyType::TRIANGLE
	});

} /* wr::ps_registry */
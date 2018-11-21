#include "d3d12_pipeline_cache.hpp"

#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_renderer.hpp"

namespace wr
{

	D3D12PipelineCache::D3D12PipelineCache(D3D12RenderSystem& render_system) : PipelineCache(), m_render_system(render_system)
	{

	}

	D3D12PipelineCache::~D3D12PipelineCache()
	{
		for (auto& pipeline : m_pipelines)
		{
			d3d12::Destroy(pipeline.second);
		}
	}

	void D3D12PipelineCache::PreparePipelines()
	{
		for (auto desc : m_pipeline_descs)
		{
			d3d12::desc::PipelineStateDesc n_desc;
			n_desc.m_counter_clockwise = desc.second.m_counter_clockwise;
			n_desc.m_cull_mode = desc.second.m_cull_mode;
			n_desc.m_depth_enabled = desc.second.m_depth_enabled;
			n_desc.m_dsv_format = desc.second.m_dsv_format;
			n_desc.m_input_layout = desc.second.m_input_layout;
			n_desc.m_num_rtv_formats = desc.second.m_num_rtv_formats;
			n_desc.m_rtv_formats = desc.second.m_rtv_formats;
			n_desc.m_topology_type = desc.second.m_topology_type;
			n_desc.m_type = desc.second.m_type;

			auto pipeline = d3d12::CreatePipelineState();
			//d3d12::SetVertexShader(pipeline, m_vertex_shader);
			//d3d12::SetFragmentShader(pipeline, m_pixel_shader);
			//d3d12::SetRootSignature(pipeline, m_root_signature);
			d3d12::FinalizePipeline(pipeline, m_render_system.m_device, n_desc);
		}
	}

} /* wr */
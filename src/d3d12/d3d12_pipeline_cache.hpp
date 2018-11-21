#pragma once

#include "../pipeline_cache.hpp"
#include <unordered_map>

namespace wr::d3d12
{
	class PipelineState;
} /* wr::d3d12 */

namespace wr
{
	class D3D12RenderSystem;

	class D3D12PipelineCache : PipelineCache
	{
	public:
		explicit D3D12PipelineCache(D3D12RenderSystem& render_system);
		~D3D12PipelineCache() final;

		void PreparePipelines() final;

	private:
		std::unordered_map<std::string, d3d12::PipelineState*> m_pipelines;
		D3D12RenderSystem& m_render_system;
	};

} /* wr */
#include "pipeline_cache.hpp"

namespace wr
{

	PipelineHandle PipelineCache::m_next_handle;
	std::unordered_map<PipelineHandle, PipelineDescription> PipelineCache::m_pipeline_descs;

	PipelineCache::PipelineCache() : m_loaded(false)
	{
	}

} /* wr */

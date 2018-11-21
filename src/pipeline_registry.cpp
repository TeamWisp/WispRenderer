#include "pipeline_registry.hpp"

namespace wr
{

	PipelineRegistry::PipelineRegistry() : Registry<PipelineRegistry, Pipeline, PipelineDescription>()
	{
	}

} /* wr */

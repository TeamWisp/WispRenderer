#pragma once

#include "../pipeline_registry.hpp"
#include "d3d12_functions.hpp"

namespace wr
{

	struct D3D12Pipeline : Pipeline
	{
		~D3D12Pipeline()
		{
			d3d12::Destroy(m_native);
		}

		d3d12::PipelineState* m_native;
	};

} /* wr */
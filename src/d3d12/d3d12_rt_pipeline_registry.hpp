#pragma once

#include "../rt_pipeline_registry.hpp"
#include "d3d12_functions.hpp"

namespace wr
{

	struct D3D12StateObject : StateObject
	{
		~D3D12StateObject()
		{
			d3d12::Destroy(m_native);
		}

		d3d12::StateObject* m_native;
	};

} /* wr */

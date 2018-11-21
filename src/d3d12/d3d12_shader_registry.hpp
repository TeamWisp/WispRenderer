#pragma once

#include "../shader_registry.hpp"
#include "d3d12_functions.hpp"

namespace wr
{

	struct D3D12Shader : Shader
	{
		~D3D12Shader()
		{
			d3d12::Destroy(m_native);
		}

		d3d12::Shader* m_native;
	};

} /* wr */
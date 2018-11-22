#pragma once

#include "../root_signature_registry.hpp"
#include "d3d12_functions.hpp"

namespace wr
{

	struct D3D12RootSignature : RootSignature
	{
		~D3D12RootSignature()
		{
			d3d12::Destroy(m_native);
		}

		d3d12::RootSignature* m_native;
	};

} /* wr */
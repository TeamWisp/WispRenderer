#pragma once

#include <d3d12.h>
#include <vector>

#include "util/defines.hpp"

namespace wr
{

	struct Vertex
	{
		float m_pos[3];

		static std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputLayout()
		{
			std::vector<D3D12_INPUT_ELEMENT_DESC> layout = {
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, m_pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			return layout;
		}
	};

	IS_PROPER_VERTEX_CLASS(Vertex)

} /* wr */
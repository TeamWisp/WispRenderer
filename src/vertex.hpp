#pragma once

#include <d3d12.h>
#include <vector>

#include "util/defines.hpp"

namespace wr
{

	//! 2D vertex (in clip space)
	struct Vertex2D 
	{
		float m_pos[2];

		static std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputLayout()
		{
			std::vector<D3D12_INPUT_ELEMENT_DESC> layout = {
				{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex2D, m_pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			};

			return layout;
		}

	};

	//! Default Vertex
	struct Vertex
	{
		float m_pos[3];
		float m_uv[2];
		float m_normal[3];

		static std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputLayout()
		{
			std::vector<D3D12_INPUT_ELEMENT_DESC> layout = {
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, m_pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, m_uv), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, m_normal), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			};

			return layout;
		}
	};

	IS_PROPER_VERTEX_CLASS(Vertex)
	IS_PROPER_VERTEX_CLASS(Vertex2D)

} /* wr */
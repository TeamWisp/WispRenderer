/*!
 * Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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
		float m_tangent[3];
		float m_bitangent[3];

		static std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputLayout()
		{
			std::vector<D3D12_INPUT_ELEMENT_DESC> layout = {
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, m_pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, m_uv), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, m_normal), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, m_tangent), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, m_bitangent), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
			};

			return layout;
		}
	};

	//! Default Vertex
	struct VertexColor
	{
		float m_pos[3];
		float m_uv[2];
		float m_normal[3];
		float m_tangent[3];
		float m_bitangent[3];
		float m_color[3];

		static std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputLayout()
		{
			std::vector<D3D12_INPUT_ELEMENT_DESC> layout = {
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(VertexColor, m_pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(VertexColor, m_uv), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(VertexColor, m_normal), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(VertexColor, m_tangent), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(VertexColor, m_bitangent), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(VertexColor, m_color), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
			};

			return layout;
		}
	};

	//! Default Vertex
	struct VertexNoTangent
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
	IS_PROPER_VERTEX_CLASS(VertexNoTangent)
	IS_PROPER_VERTEX_CLASS(Vertex2D)

} /* wr */
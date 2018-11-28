#include "vertex.hpp"
#include <array>

#undef max
#undef min

namespace wr
{
	std::vector<D3D12_INPUT_ELEMENT_DESC> ClipVertex2D::GetInputLayout()
	{
		std::vector<D3D12_INPUT_ELEMENT_DESC> layout = {
			{ "POSITION", 0, DXGI_FORMAT_R8G8_SNORM, 0, offsetof(ClipVertex2D, m_pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		return layout;
	}


	std::vector<D3D12_INPUT_ELEMENT_DESC> Vertex2D::GetInputLayout()
	{
		std::vector<D3D12_INPUT_ELEMENT_DESC> layout = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex2D, m_pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		return layout;
	}

	std::vector<D3D12_INPUT_ELEMENT_DESC> Vertex::GetInputLayout()
	{
		std::vector<D3D12_INPUT_ELEMENT_DESC> layout = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, m_pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, m_uv), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, m_normal), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		return layout;
	}

	std::vector<D3D12_INPUT_ELEMENT_DESC> CompressedVertex::GetInputLayout()
	{
		std::vector<D3D12_INPUT_ELEMENT_DESC> layout = {
			{ "POSITION", 0, DXGI_FORMAT_R16G16B16A16_UNORM, 0, offsetof(CompressedVertex, m_pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R16G16B16A16_UNORM, 0, offsetof(CompressedVertex, m_normal), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		return layout;
	}

	void CompressedVertex::CompressNormal(Vertex& in_vertex, CompressedVertex& out_vertex) 
	{
		std::array<float, 3> v;
		memcpy(v.data(), &in_vertex.m_normal, 12);

		out_vertex.m_normal[0] = uint16_t((v[0] * 0.5 + 0.5) * 65535);
		out_vertex.m_normal[1] = uint16_t((v[1] * 0.5 + 0.5) * 65535);
		out_vertex.m_normal[2] = uint16_t((v[2] * 0.5 + 0.5) * 65535);

	}

	void CompressedVertex::CompressPositionAndUv(Vertex& in_vertex, CompressedVertex& out_vertex, CompressedVertex::Details& compression_details)
	{
		float* pos = in_vertex.m_pos;
		float* pos_start = compression_details.m_pos_start;
		float* pos_length = compression_details.m_pos_length;

		out_vertex.m_pos[0] = uint16_t((pos[0] - pos_start[0]) / pos_length[0] * 65535);
		out_vertex.m_pos[1] = uint16_t((pos[1] - pos_start[1]) / pos_length[1] * 65535);
		out_vertex.m_pos[2] = uint16_t((pos[2] - pos_start[2]) / pos_length[2] * 65535);

		float* uv = in_vertex.m_pos;
		float* uv_start = compression_details.m_pos_start;
		float* uv_length = compression_details.m_pos_length;

		out_vertex.m_uv_x = uint16_t((uv[0] - uv_start[0]) / uv_length[0] * 65535);
		out_vertex.m_uv_y = uint16_t((uv[1] - uv_start[1]) / uv_length[1] * 65535);
	}

	void CompressedVertex::CompressVertices(std::vector<Vertex>& in_vertices, std::vector<CompressedVertex>& out_vertices, CompressedVertex::Details& compression_details)
	{
		out_vertices.resize(in_vertices.size());

		float pos_end[3];
		float uv_end[2];

		float* pos_start = compression_details.m_pos_start;
		float* uv_start = compression_details.m_uv_start;

		//Setup compression details as max bounds (so first vertex becomes min & max)
		if (!compression_details.m_is_defined)
		{
			float* pos_start = compression_details.m_pos_start;
			float* uv_start = compression_details.m_uv_start;

			float f32_max = std::numeric_limits<float>::max();
			pos_start[0] = pos_start[1] = pos_start[2] = uv_start[0] = uv_start[1] = f32_max;

			float f32_min = std::numeric_limits<float>::min();
			pos_end[0] = pos_end[1] = pos_end[2] = uv_end[0] = uv_end[1] = f32_min;
		}

		uint32_t i, size = (uint32_t) in_vertices.size();

		//Compress normals
		//Compress uv & position if compression details is defined; otherwise calculate min/max bounds for UV & position

		for (i = 0; i < size; ++i)
		{
			Vertex& in_vertex = in_vertices[i];
			CompressedVertex& out_vertex = out_vertices[i];

			CompressNormal(in_vertex, out_vertex);

			if (compression_details.m_is_defined)
				CompressPositionAndUv(in_vertex, out_vertex, compression_details);
			else
			{
				float* pos = in_vertex.m_pos;
				float* uv = in_vertex.m_uv;

				for (uint32_t j = 0; j < 3; ++j)
				{
					float& posj = pos[j];

					if (posj < pos_start[j])
						pos_start[j] = posj;
					
					if (posj > pos_end[j])
						pos_end[j] = posj;

					if (j == 2) break;

					float& uvj = uv[j];

					if (uvj < uv_start[j])
						uv_start[j] = uvj;

					if (uvj > uv_end[j])
						uv_end[j] = uvj;
				}
			}
		}

		//Set max boundries for compression details and compress position & uv

		if (!compression_details.m_is_defined)
		{
			compression_details.m_is_defined = 1;
			compression_details.m_pos_length[0] = pos_end[0] - pos_start[0];
			compression_details.m_pos_length[1] = pos_end[1] - pos_start[1];
			compression_details.m_pos_length[2] = pos_end[2] - pos_start[2];
			compression_details.m_uv_length[0] = uv_end[0] - uv_start[0];
			compression_details.m_uv_length[1] = uv_end[1] - uv_start[1];

			for (i = 0; i < size; ++i)
			{
				Vertex& in_vertex = in_vertices[i];
				CompressedVertex& out_vertex = out_vertices[i];

				CompressPositionAndUv(in_vertex, out_vertex, compression_details);
			}
		}

	}

}
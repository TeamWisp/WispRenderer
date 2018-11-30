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
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, m_normal), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, m_tangent), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, m_bitangent), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		return layout;
	}

	std::vector<D3D12_INPUT_ELEMENT_DESC> CompressedVertex::GetInputLayout()
	{
		std::vector<D3D12_INPUT_ELEMENT_DESC> layout = {
			{ "POSITION", 0, DXGI_FORMAT_R16G16B16A16_UNORM, 0, offsetof(CompressedVertex, m_pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R16G16B16A16_UNORM, 0, offsetof(CompressedVertex, m_normal), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R16G16B16A16_UNORM, 0, offsetof(CompressedVertex, m_tangent), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BITANGENT", 0, DXGI_FORMAT_R16G16_UNORM, 0, offsetof(CompressedVertex, m_bitangent) + 2, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		return layout;
	}

	void CompressedVertex::CompressDirection(float* v, uint16_t* dst)
	{
		dst[0] = uint16_t((v[0] * 0.5 + 0.5) * 65535);
		dst[1] = uint16_t((v[1] * 0.5 + 0.5) * 65535);
		dst[2] = uint16_t((v[2] * 0.5 + 0.5) * 65535);
	}

	void CompressedVertex::CompressTBN(Vertex& in_vertex, CompressedVertex& out_vertex)
	{
		CompressDirection(in_vertex.m_normal, out_vertex.m_normal);
		CompressDirection(in_vertex.m_tangent, out_vertex.m_tangent);
		CompressDirection(in_vertex.m_bitangent, out_vertex.m_bitangent);
	}

	void CompressedVertex::CompressPosition(float* pos, uint16_t* out_pos, CompressedVertex::Details& compression_details)
	{
		float* pos_start = compression_details.m_pos_start;
		float* pos_length = compression_details.m_pos_length;

		out_pos[0] = uint16_t((pos[0] - pos_start[0]) / pos_length[0] * 65535);
		out_pos[1] = uint16_t((pos[1] - pos_start[1]) / pos_length[1] * 65535);
		out_pos[2] = uint16_t((pos[2] - pos_start[2]) / pos_length[2] * 65535);
	}

	void CompressedVertex::CompressUv(float* uv, uint16_t& x, uint16_t& y, Details& compression_details)
	{
		float* uv_start = compression_details.m_pos_start;
		float* uv_length = compression_details.m_pos_length;

		x = uint16_t((uv[0] - uv_start[0]) / uv_length[0] * 65535);
		y = uint16_t((uv[1] - uv_start[1]) / uv_length[1] * 65535);
	}

	void CompressedVertex::CompressPositionAndUv(Vertex& in_vertex, CompressedVertex& out_vertex, CompressedVertex::Details& compression_details)
	{
		CompressPosition(in_vertex.m_pos, out_vertex.m_pos, compression_details);
		CompressUv(in_vertex.m_uv, out_vertex.m_uv_x, out_vertex.m_uv_y, compression_details);
	}

	void CompressedVertex::DetectBoundsPosition(float* pos, float* min_pos, float* max_pos)
	{
		if (pos[0] > max_pos[0])
			max_pos[0] = pos[0];

		if (pos[0] < min_pos[0])
			min_pos[0] = pos[0];

		if (pos[1] > max_pos[1])
			max_pos[1] = pos[1];

		if (pos[1] < min_pos[1])
			min_pos[1] = pos[1];

		if (pos[2] > max_pos[2])
			max_pos[2] = pos[2];

		if (pos[2] < min_pos[2])
			min_pos[2] = pos[2];
	}

	void CompressedVertex::DetectBoundsUv(float* uv, float* min_uv, float* max_uv)
	{
		if (uv[0] > max_uv[0])
			max_uv[0] = uv[0];

		if (uv[0] < min_uv[0])
			min_uv[0] = uv[0];

		if (uv[1] > max_uv[1])
			max_uv[1] = uv[1];

		if (uv[1] < min_uv[1])
			min_uv[1] = uv[1];
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

			CompressTBN(in_vertex, out_vertex);

			if (compression_details.m_is_defined)
				CompressPositionAndUv(in_vertex, out_vertex, compression_details);
			else
			{
				float* pos = in_vertex.m_pos;
				float* uv = in_vertex.m_uv;

				DetectBoundsPosition(pos, pos_start, pos_end);
				DetectBoundsUv(uv, uv_start, uv_end);
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
#pragma once

#include "scene_graph.hpp"

namespace wr
{

	struct MeshNode : Node
	{
		Model* m_model;

		DirectX::XMVECTOR m_aabb[2];

		MeshNode(Model* model) : m_model(model) { }

		void Update(uint32_t frame_idx)
		{
			//Reset AABB

			m_aabb[0] = {
				std::numeric_limits<float>::max(),
				std::numeric_limits<float>::max(),
				std::numeric_limits<float>::max()
			};

			m_aabb[1] = {
				-std::numeric_limits<float>::max(),
				-std::numeric_limits<float>::max(),
				-std::numeric_limits<float>::max()
			};

			//Transform all coords from model to world space
			//Pick the min/max bounds

			for (DirectX::XMVECTOR& vec : m_model->m_box)
			{
				DirectX::XMVECTOR tvec = DirectX::XMVector4Transform(vec, m_transform);

				m_aabb[0] = {
					std::min(tvec.m128_f32[0], *m_aabb[0].m128_f32),
					std::min(tvec.m128_f32[1], m_aabb[0].m128_f32[1]),
					std::min(tvec.m128_f32[2], m_aabb[0].m128_f32[2]),
					1
				};

				m_aabb[1] = {
					std::max(tvec.m128_f32[0], *m_aabb[1].m128_f32),
					std::max(tvec.m128_f32[1], m_aabb[1].m128_f32[1]),
					std::max(tvec.m128_f32[2], m_aabb[1].m128_f32[2]),
					1
				};

			}

			SignalUpdate(frame_idx);
		}
	};

} /* wr */
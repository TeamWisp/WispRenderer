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
			m_aabb[0] = DirectX::XMVector4Transform(m_model->m_aabb[0], m_transform);
			m_aabb[1] = DirectX::XMVector4Transform(m_model->m_aabb[1], m_transform);
			SignalUpdate(frame_idx);
		}
	};

} /* wr */
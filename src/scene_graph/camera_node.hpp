#pragma once

#include "scene_graph.hpp"

namespace wr
{

	struct CameraNode : Node
	{
		DECL_SUBNODE(CameraNode);

		CameraNode(float fov_deg, float aspect_ratio)
			: Node(),
			m_active(true),
			m_pos{ 0, 0, 0},
			m_euler{ 0, 0, 0 },
			m_frustum_near(0.1f),
			m_frustum_far(640.f),
			m_fov(fov_deg / 180 * 3.1415926535f),
			m_aspect_ratio(aspect_ratio)
		{
			SUBMODE_CONSTRUCTOR
		}

		void SetPosition(float x, float y, float z);
		void SetFov(float deg);

		void UpdateTemp(unsigned int frame_idx);

		bool m_active;

		float m_pos[3];
		float m_euler[3];

		float m_frustum_near;
		float m_frustum_far;
		float m_fov;
		float m_aspect_ratio;

		DirectX::XMMATRIX m_view;
		DirectX::XMMATRIX m_projection;
		DirectX::XMMATRIX m_inverse_projection;

		ConstantBufferHandle* m_camera_cb;
	};

} /* wr */
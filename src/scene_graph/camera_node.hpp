#pragma once

#include "scene_graph.hpp"

namespace wr
{

	struct MeshNode;

	struct CameraNode : Node
	{
		CameraNode(float fov_deg, float aspect_ratio)
			: Node(),
			m_active(true),
			m_frustum_near(0.1f),
			m_frustum_far(250.f),
			m_fov(fov_deg / 180 * 3.1415926535f),
			m_aspect_ratio(aspect_ratio)
		{
		}

		void SetFov(float deg);

		void SetAspectRatio(float ratio);

		void UpdateTemp(unsigned int frame_idx);

		bool InView(std::shared_ptr<MeshNode>& node);

		void CalculatePlanes();

		bool m_active;

		float m_frustum_near;
		float m_frustum_far;
		float m_fov;
		float m_aspect_ratio;

		DirectX::XMMATRIX m_view;
		DirectX::XMMATRIX m_projection;
		DirectX::XMMATRIX m_view_projection;
		DirectX::XMMATRIX m_inverse_projection;
		DirectX::XMMATRIX m_inverse_view;

		DirectX::XMVECTOR m_planes[6];

		ConstantBufferHandle* m_camera_cb;
	};

} /* wr */
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
			m_frustum_far(1000.0f),
			m_fov(2.0f * std::atan2(35 / aspect_ratio, 2.0f * 30.f)),
			m_aspect_ratio(aspect_ratio),
			m_focal_length(30.0f),
			m_film_size(35.0f),
			m_f_number(4.2f),
			m_aperture_blades(5),
			m_focus_dist(0)
		{
		}

		void SetFov(float deg);

		void SetFovFromFocalLength(float aspect_ratio, float filmSize);

		void SetAspectRatio(float ratio);

		void SetFocalLength(float length);

		void UpdateTemp(unsigned int frame_idx);

		bool InView(std::shared_ptr<MeshNode>& node);

		void CalculatePlanes();

		bool m_active;

		float m_frustum_near;
		float m_frustum_far;
		float m_fov;
		float m_aspect_ratio;
		float m_focal_length;
		float m_film_size;
		float m_f_number;
		float m_focus_dist;
		int m_aperture_blades;
		bool m_enable_dof = false;

		DirectX::XMMATRIX m_view;
		DirectX::XMMATRIX m_projection;
		DirectX::XMMATRIX m_view_projection;
		DirectX::XMMATRIX m_inverse_projection;
		DirectX::XMMATRIX m_inverse_view;

		DirectX::XMVECTOR m_planes[6];

		ConstantBufferHandle* m_camera_cb;
	};

} /* wr */
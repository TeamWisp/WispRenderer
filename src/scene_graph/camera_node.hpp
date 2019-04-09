#pragma once

#include "scene_graph.hpp"
#include "../util/named_type.hpp"

namespace wr
{

	struct MeshNode;

	struct CameraNode : Node
	{
		using FovDefault = util::NamedType<float>;
		using FovFocalLength = util::NamedType<float>;
		using FovAspectRatio = util::NamedType<float>;
		using FovFilmSize = util::NamedType<float>;

		struct FoV
		{
			FoV(FovDefault deg) : m_fov(deg.Get() / 180.0f * 3.1415926535f)
			{
			}

			FoV(FovFocalLength focal_length, FovAspectRatio aspect_ratio, FovFilmSize film_size) :
				m_fov(2.0f * std::atan2(film_size.Get() / aspect_ratio.Get(), 2.0f * focal_length.Get()))
			{
			}

			float m_fov;
		};

		CameraNode(float fov_deg, float aspect_ratio)
			: Node(),
			m_active(true),
			m_frustum_near(0.1f),
			m_frustum_far(10000.0f),
			m_aspect_ratio(aspect_ratio),
			m_focal_length(35.0f),
			m_film_size(45.0f),
			m_fov(FovFocalLength(m_focal_length), FovAspectRatio(aspect_ratio), FovFilmSize(m_film_size)),
			m_f_number(32.0f),
			m_shape_amt(0.0f),
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
		float m_aspect_ratio;
		float m_focal_length;
		float m_film_size;
		float m_f_number;
		float m_focus_dist;
		float m_shape_amt;
		int m_aperture_blades;
		bool m_enable_dof = false;

		FoV m_fov;

		DirectX::XMMATRIX m_view;
		DirectX::XMMATRIX m_projection;
		DirectX::XMMATRIX m_prev_projection;
		DirectX::XMMATRIX m_prev_view;
		DirectX::XMMATRIX m_view_projection;
		DirectX::XMMATRIX m_inverse_projection;
		DirectX::XMMATRIX m_inverse_view;

		DirectX::XMVECTOR m_planes[6];

		ConstantBufferHandle* m_camera_cb;
	};

} /* wr */
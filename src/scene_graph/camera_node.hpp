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

#include "node.hpp"

#include <array>

#include "../util/named_type.hpp"
#include "../constant_buffer_pool.hpp"

namespace wr
{

	struct MeshNode;

	struct CameraNode : Node
	{
		using FovDefault = util::NamedType<float>;
		using FovFocalLength = util::NamedType<float>;
		using FovAspectRatio = util::NamedType<float>;
		using FovFilmSize = util::NamedType<float>;
		using OrthographicWidth = util::NamedType<int>;
		using OrthographicHeight = util::NamedType<int>;

		struct FoV
		{
			explicit FoV(FovDefault deg) : m_fov(deg.Get() / 180.0f * 3.1415926535f)
			{
			}

			FoV(FovFocalLength focal_length, FovAspectRatio aspect_ratio, FovFilmSize film_size) :
				m_fov(2.0f * std::atan2(film_size.Get() / aspect_ratio.Get(), 2.0f * focal_length.Get()))
			{
			}

			float m_fov;
		};

		struct OrthographicResolution
		{
			explicit OrthographicResolution(OrthographicWidth width, OrthographicHeight height) : m_width(width), m_height(height * -1)
			{
			}

			int m_width;
			int m_height;
		};

		CameraNode(float aspect_ratio)
			: Node(typeid(CameraNode)),
			m_active(true),
			m_frustum_near(0.1f),
			m_frustum_far(10000.0f),
			m_aspect_ratio(aspect_ratio),
			m_focal_length(35.0f),
			m_film_size(45.0f),
			m_fov(FovFocalLength(m_focal_length), FovAspectRatio(aspect_ratio), FovFilmSize(m_film_size)),
			m_f_number(32.0f),
			m_shape_amt(0.5f),
			m_aperture_blades(5),
			m_focus_dist(0),
			m_override_projection(false),
			m_projection_offset_x(0),
			m_projection_offset_y(0),
			m_view(),
			m_inverse_view(),
			m_projection(),
			m_inverse_projection(),
			m_view_projection(),
			m_camera_cb(),
			m_planes(),
			m_ortho_res(OrthographicWidth(1280), OrthographicHeight(720))
		{
		}

		void SetFov(float deg);
		void SetFovFromFocalLength(float aspect_ratio, float filmSize);
		void SetAspectRatio(float ratio);
		void SetFocalLength(float length);
		void SetFrustumNear(float value) noexcept;
		void SetFrustumFar(float value) noexcept;
		void SetProjectionOffset(float x, float y);

		void SetOrthographicResolution(std::uint32_t width, std::uint32_t height);

		std::pair<float, float> GetProjectionOffset();
		
		void UpdateTemp(unsigned int frame_idx);
		bool InView(const std::shared_ptr<MeshNode>& node) const;
		bool InRange(const std::shared_ptr<MeshNode>& node, const float dist) const;
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
		bool m_override_projection = false;
		bool m_enable_orthographic = false;

		FoV m_fov;
		OrthographicResolution m_ortho_res;

		DirectX::XMMATRIX m_view;
		DirectX::XMMATRIX m_projection;
		DirectX::XMMATRIX m_prev_projection;
		DirectX::XMMATRIX m_prev_view;
		DirectX::XMMATRIX m_view_projection;
		DirectX::XMMATRIX m_inverse_projection;
		DirectX::XMMATRIX m_inverse_view;
		float m_projection_offset_x; // Used By Ansel For Super Resolution
		float m_projection_offset_y; // Used By Ansel For Super Resolution

		std::array<DirectX::XMVECTOR, 6> m_planes;

		ConstantBufferHandle* m_camera_cb;

		int m_window_resolution[2] = { 0,0 };
	};

} /* wr */

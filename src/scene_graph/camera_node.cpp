#include "camera_node.hpp"

#include "../util/aabb.hpp"
#include "mesh_node.hpp"

namespace wr
{

	void CameraNode::SetFov(float deg)
	{
		m_fov.m_fov = deg / 180.0f * 3.1415926535f;
		SignalChange();
	}

	void CameraNode::SetFovFromFocalLength(float aspect_ratio, float filmSize)
	{
		float verticalSize = filmSize / aspect_ratio;
		m_fov.m_fov = 2.0f * std::atan2(verticalSize, 2.0f * m_focal_length);
		SignalChange();
	}

	void CameraNode::SetAspectRatio(float ratio)
	{
		m_aspect_ratio = ratio;
		SignalChange();
	}

	void CameraNode::SetFrustumNear(float value) noexcept
	{
		m_frustum_near = value;
		SignalChange();
	}

	void CameraNode::SetFrustumFar(float value) noexcept
	{
		m_frustum_far = value;
		SignalChange();
	}

	void CameraNode::SetFocalLength(float length)
	{
		m_focal_length = length;
		SetFovFromFocalLength(m_aspect_ratio, m_film_size);
		SignalChange();
	}

	void CameraNode::SetProjectionOffset(float x, float y)
	{
		m_projection_offset_x = x;
		m_projection_offset_y = y;
	}

	std::pair<float, float> CameraNode::GetProjectionOffset()
	{
		return std::pair<float, float>(m_projection_offset_x, m_projection_offset_y);
	}

	void CameraNode::SetOrthographicResolution(std::uint32_t width, std::uint32_t height)
	{
		m_ortho_res.m_width = static_cast<int>(width);

		//Window resolution counts in negative on Y axis, therefore conversion to signed is needed.
		m_ortho_res.m_height = static_cast<int>(height) * -1;

		SignalChange();
	}

	void CameraNode::UpdateTemp(unsigned int frame_idx)
	{
		DirectX::XMVECTOR pos = { m_transform.r[3].m128_f32[0], m_transform.r[3].m128_f32[1], m_transform.r[3].m128_f32[2] };

		DirectX::XMVECTOR up = DirectX::XMVector3Normalize(m_transform.r[1]);
		DirectX::XMVECTOR forward = DirectX::XMVectorNegate(DirectX::XMVector3Normalize(m_transform.r[2]));
		DirectX::XMVECTOR right = DirectX::XMVector3Normalize(m_transform.r[0]);

		m_prev_view = m_view;
		m_prev_projection = m_projection;

		m_view = DirectX::XMMatrixLookToRH(pos, forward, up);
		
		if (!m_override_projection)
		{
			m_projection = DirectX::XMMatrixPerspectiveFovRH(m_fov.m_fov, m_aspect_ratio, m_frustum_near, m_frustum_far);

			if (m_enable_orthographic)
			{
				m_projection = DirectX::XMMatrixOrthographicOffCenterRH(
					0,
					static_cast<float>(m_ortho_res.m_width), 
					static_cast<float>(m_ortho_res.m_height), 
					0,
					m_frustum_near,
					m_frustum_far);
			}

			m_projection.r[2].m128_f32[0] += m_projection_offset_x;
			m_projection.r[2].m128_f32[1] += m_projection_offset_y;
		}

		m_view_projection = m_view * m_projection;
		m_inverse_projection = DirectX::XMMatrixInverse(nullptr, m_projection);
		m_inverse_view = DirectX::XMMatrixInverse(nullptr, m_view);

		CalculatePlanes();

		SignalUpdate(frame_idx);
	}

	//Frustum culling code;
	//optimized and refactored version of
	//https://www.braynzarsoft.net/viewtutorial/q16390-34-aabb-cpu-side-frustum-culling
	void CameraNode::CalculatePlanes()
	{
		//Left plane

		m_planes[0] = DirectX::XMPlaneNormalize({
			m_view_projection.r[0].m128_f32[3] + *m_view_projection.r[0].m128_f32,
			m_view_projection.r[1].m128_f32[3] + *m_view_projection.r[1].m128_f32,
			m_view_projection.r[2].m128_f32[3] + *m_view_projection.r[2].m128_f32,
			m_view_projection.r[3].m128_f32[3] + *m_view_projection.r[3].m128_f32
			});

		//Right plane

		m_planes[1] = DirectX::XMPlaneNormalize({
			m_view_projection.r[0].m128_f32[3] - *m_view_projection.r[0].m128_f32,
			m_view_projection.r[1].m128_f32[3] - *m_view_projection.r[1].m128_f32,
			m_view_projection.r[2].m128_f32[3] - *m_view_projection.r[2].m128_f32,
			m_view_projection.r[3].m128_f32[3] - *m_view_projection.r[3].m128_f32
			});

		//Top plane

		m_planes[2] = DirectX::XMPlaneNormalize({
			m_view_projection.r[0].m128_f32[3] - m_view_projection.r[0].m128_f32[1],
			m_view_projection.r[1].m128_f32[3] - m_view_projection.r[1].m128_f32[1],
			m_view_projection.r[2].m128_f32[3] - m_view_projection.r[2].m128_f32[1],
			m_view_projection.r[3].m128_f32[3] - m_view_projection.r[3].m128_f32[1]
			});

		//Bottom plane

		m_planes[3] = DirectX::XMPlaneNormalize({
			m_view_projection.r[0].m128_f32[3] + m_view_projection.r[0].m128_f32[1],
			m_view_projection.r[1].m128_f32[3] + m_view_projection.r[1].m128_f32[1],
			m_view_projection.r[2].m128_f32[3] + m_view_projection.r[2].m128_f32[1],
			m_view_projection.r[3].m128_f32[3] + m_view_projection.r[3].m128_f32[1]
			});

		//Near plane

		m_planes[4] = DirectX::XMPlaneNormalize({
			m_view_projection.r[0].m128_f32[2],
			m_view_projection.r[1].m128_f32[2],
			m_view_projection.r[2].m128_f32[2],
			m_view_projection.r[3].m128_f32[2]
			});

		//Far plane

		m_planes[5] = DirectX::XMPlaneNormalize({
			m_view_projection.r[0].m128_f32[3] - m_view_projection.r[0].m128_f32[2],
			m_view_projection.r[1].m128_f32[3] - m_view_projection.r[1].m128_f32[2],
			m_view_projection.r[2].m128_f32[3] - m_view_projection.r[2].m128_f32[2],
			m_view_projection.r[3].m128_f32[3] - m_view_projection.r[3].m128_f32[2]
		});

	}

	bool CameraNode::InView(std::shared_ptr<MeshNode>& node)
	{
		AABB aabb = node->m_aabb;
		return aabb.InFrustum(m_planes);
	}

} /* wr */

#include "camera_node.hpp"

namespace wr
{

	void CameraNode::SetFov(float deg)
	{
		m_fov = deg / 180 * 3.1415926535f;
		SignalChange();
	}

	void CameraNode::UpdateTemp(unsigned int frame_idx)
	{
		DirectX::XMVECTOR pos = { m_transform.r[3].m128_f32[0], m_transform.r[3].m128_f32[1], m_transform.r[3].m128_f32[2] };

		DirectX::XMVECTOR up = DirectX::XMVector3Normalize(m_transform.r[1]);
		DirectX::XMVECTOR forward = DirectX::XMVector3Normalize(m_transform.r[2]);
		DirectX::XMVECTOR right = DirectX::XMVector3Normalize(m_transform.r[0]);

		m_view = DirectX::XMMatrixLookAtRH(pos, DirectX::XMVectorAdd(pos, forward), up);
		m_projection = DirectX::XMMatrixPerspectiveFovRH(m_fov, m_aspect_ratio, m_frustum_near, m_frustum_far);
		m_inverse_projection = DirectX::XMMatrixInverse(nullptr, m_projection);

		SignalUpdate(frame_idx);
	}

} /* wr */
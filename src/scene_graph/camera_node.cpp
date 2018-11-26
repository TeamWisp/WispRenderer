#include "camera_node.hpp"

namespace wr
{

	void CameraNode::SetPosition(float x, float y, float z)
	{
		m_pos[0] = x;
		m_pos[1] = y;
		m_pos[2] = z;

		SignalChange();
	}

	void CameraNode::SetFov(float deg)
	{

		m_fov = deg / 180 * 3.1415926535f;

		SignalChange();

	}

	void CameraNode::UpdateTemp(unsigned int frame_idx)
	{
		m_requires_update[frame_idx] = false;

		DirectX::XMVECTOR pos{ m_pos[0], m_pos[1], m_pos[2] };

		DirectX::XMVECTOR forward{
			sin(m_euler[1]) * cos(m_euler[0]),
			sin(m_euler[0]),
			cos(m_euler[1]) * cos(m_euler[0]),
		};
		forward = DirectX::XMVector3Normalize(forward);

		auto right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(forward, { world_up[0], world_up[1], world_up[2] }));
		auto up = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(right, forward));

		m_view = DirectX::XMMatrixLookAtRH(pos, DirectX::XMVectorAdd(pos, forward), up);
		m_projection = DirectX::XMMatrixPerspectiveFovRH(m_fov, m_aspect_ratio, m_frustum_near, m_frustum_far);
		m_inverse_projection = DirectX::XMMatrixInverse(nullptr, m_projection);
	}

} /* wr */
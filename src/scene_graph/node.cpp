#include "scene_graph.hpp"

namespace wr
{
	Node::Node()
	{
		SignalChange();
	}

	void Node::SignalChange(bool cascade)
	{
		m_requires_update[0] = m_requires_update[1] = m_requires_update[2] = true;

		if (cascade)
		{
			for (std::shared_ptr<Node>& child : m_children)
			{
				child->SignalChange(true);
			}
		}
	}

	void Node::SignalUpdate(unsigned int frame_idx)
	{
		m_requires_update[frame_idx] = false;
	}

	bool Node::RequiresUpdate(unsigned int frame_idx)
	{
		return m_requires_update[frame_idx];
	}

	void Node::SetRotation(DirectX::XMVECTOR roll_pitch_yaw_deg)
	{
		m_rotation_deg = roll_pitch_yaw_deg;
		SignalChange(true);
	}

	void Node::SetPosition(DirectX::XMVECTOR position)
	{
		m_position = position;
		SignalChange(true);
	}

	void Node::SetScale(DirectX::XMVECTOR scale)
	{
		m_scale = scale;
		SignalChange(true);
	}

	void Node::SetTransform(DirectX::XMVECTOR position, DirectX::XMVECTOR rotation_deg, DirectX::XMVECTOR scale)
	{
		SetPosition(position);
		SetRotation(rotation_deg);
		SetScale(scale);
	}

	void Node::UpdateTransform()
	{
		float to_deg = 3.1415926435f / 180.f;
		DirectX::XMVECTOR rotation = DirectX::XMVectorMultiply(m_rotation_deg, { to_deg, to_deg, to_deg });
		m_rotation = DirectX::XMQuaternionRotationRollPitchYawFromVector(rotation);

		DirectX::XMMATRIX translation_mat = DirectX::XMMatrixTranslationFromVector(m_position);
		DirectX::XMMATRIX rotation_mat = DirectX::XMMatrixRotationQuaternion(m_rotation);
		DirectX::XMMATRIX scale_mat = DirectX::XMMatrixScalingFromVector(m_scale);
		m_transform = m_local_transform = scale_mat * rotation_mat * translation_mat;

		if (m_parent)
			m_transform *= m_parent->m_transform;
	}

}
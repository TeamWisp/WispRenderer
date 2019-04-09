#include "node.hpp"

namespace wr
{
	Node::Node()
	{
		SignalTransformChange();
	}

	void Node::SignalChange()
	{
		m_requires_update[0] = m_requires_update[1] = m_requires_update[2] = true;
	}

	void Node::SignalTransformChange()
	{
		m_requires_transform_update[0] = m_requires_transform_update[1] = m_requires_transform_update[2] = true;

		for (std::shared_ptr<Node>& child : m_children)
		{
			child->SignalTransformChange();
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

	void Node::SignalTransformUpdate(unsigned int frame_idx)
	{
		m_requires_transform_update[frame_idx] = false;
	}

	bool Node::RequiresTransformUpdate(unsigned int frame_idx)
	{
		return m_requires_transform_update[frame_idx];
	}

	void Node::SetRotation(DirectX::XMVECTOR roll_pitch_yaw)
	{
		m_rotation_radians = roll_pitch_yaw;
		SignalTransformChange();
	}

	void Node::SetPosition(DirectX::XMVECTOR position)
	{
		m_position = position;
		SignalTransformChange();
	}

	void Node::SetScale(DirectX::XMVECTOR scale)
	{
		m_scale = scale;
		SignalTransformChange();
	}

	void Node::SetTransform(DirectX::XMVECTOR position, DirectX::XMVECTOR rotation, DirectX::XMVECTOR scale)
	{
		SetPosition(position);
		SetRotation(rotation);
		SetScale(scale);
	}

	void Node::UpdateTransform()
	{
		m_prev_transform = m_transform;

		m_rotation = DirectX::XMQuaternionRotationRollPitchYawFromVector(m_rotation_radians);
		DirectX::XMMATRIX translation_mat = DirectX::XMMatrixTranslationFromVector(m_position);
		DirectX::XMMATRIX rotation_mat = DirectX::XMMatrixRotationQuaternion(m_rotation);
		DirectX::XMMATRIX scale_mat = DirectX::XMMatrixScalingFromVector(m_scale);
		m_transform = m_local_transform = scale_mat * rotation_mat * translation_mat;

		if (m_parent)
			m_transform *= m_parent->m_transform;

		SignalChange();
	}

}
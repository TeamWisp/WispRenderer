// Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "node.hpp"

#include "../util/log.hpp"

namespace wr
{
	Node::Node() : m_type_info(typeid(Node))
	{
		SignalTransformChange();
	}

	Node::Node(std::type_info const & type_info) : m_type_info(type_info)
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
		m_use_quaternion = false;
		SignalTransformChange();
	}

	void Node::SetRotationQuaternion(DirectX::XMVECTOR rotation)
	{
		m_rotation = rotation;
		m_use_quaternion = true;
		SignalTransformChange();
	}

	void Node::SetQuaternionRotation( float x, float y, float z, float w )
	{
		m_rotation = { x,y,z,w };
		m_use_quaternion = true;
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
		if (!m_use_quaternion)
		{
			m_rotation = DirectX::XMQuaternionRotationRollPitchYawFromVector(m_rotation_radians);
		}
		m_prev_transform = m_transform;

		DirectX::XMMATRIX translation_mat = DirectX::XMMatrixTranslationFromVector(m_position);
		DirectX::XMMATRIX rotation_mat = DirectX::XMMatrixRotationQuaternion(m_rotation);
		DirectX::XMMATRIX scale_mat = DirectX::XMMatrixScalingFromVector(m_scale);
		m_transform = m_local_transform = scale_mat * rotation_mat * translation_mat;

		if (m_parent)
			m_transform *= m_parent->m_transform;

		SignalChange();
	}

} /* wr */

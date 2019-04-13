#pragma once

#include <bitset>
#include <functional>
#include <memory>
#include <DirectXMath.h>

namespace wr
{
	struct Node : std::enable_shared_from_this<Node>
	{
		Node();
		Node(std::type_info const & type_info);

		void SignalChange();
		void SignalUpdate(unsigned int frame_idx);
		bool RequiresUpdate(unsigned int frame_idx);

		void SignalTransformChange();
		void SignalTransformUpdate(unsigned int frame_idx);
		bool RequiresTransformUpdate(unsigned int frame_idx);

		//Takes roll, pitch and yaw and converts it to quaternion
		virtual void SetRotation(DirectX::XMVECTOR roll_pitch_yaw);
		virtual void SetRotationQuaternion(DirectX::XMVECTOR rotation);

		//Takes raw values of a quaternion
		virtual void SetQuaternionRotation( float x, float y, float z, float w );

		//Sets position
		virtual void SetPosition(DirectX::XMVECTOR position);

		//Sets scale
		virtual void SetScale(DirectX::XMVECTOR scale);

		//Position, rotation (roll, pitch, yaw) and scale
		virtual void SetTransform(DirectX::XMVECTOR position, DirectX::XMVECTOR rotation, DirectX::XMVECTOR scale);

		//Update the transform; done automatically when SignalChange is called
		void UpdateTransform();

		std::shared_ptr<Node> m_parent;
		std::vector<std::shared_ptr<Node>> m_children;

		//Translation of mesh node
		DirectX::XMVECTOR m_position = { 0, 0, 0, 1 };

		//Rotation as quaternion
		DirectX::XMVECTOR m_rotation;

		//Rotation in radians
		DirectX::XMVECTOR m_rotation_radians = { 0,0,0 };

		//Scale
		DirectX::XMVECTOR m_scale = { 1, 1, 1, 0 };

		//Transformation
		DirectX::XMMATRIX m_local_transform, m_transform;

		const std::type_info& m_type_info;

	protected:
		bool m_use_quaternion = false;

	private:
		std::bitset<3> m_requires_update;
		std::bitset<3> m_requires_transform_update;
		bool m_used_quaternion;
	};
} // namespace wr
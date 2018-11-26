#pragma once

#include "scene_graph.hpp"

namespace wr
{

	struct MeshNode : Node
	{
		MeshNode(Model* model);

		//Takes roll, pitch and yaw from degrees and converts it to quaternion
		void SetRotation(DirectX::XMVECTOR roll_pitch_yaw_deg);

		//Sets position
		void SetPosition(DirectX::XMVECTOR position);

		//Sets scale
		void SetScale(DirectX::XMVECTOR scale);

		//Position, rotation in degrees (roll, pitch, yaw) and scale
		void SetTransform(DirectX::XMVECTOR position, DirectX::XMVECTOR rotation_deg, DirectX::XMVECTOR scale);

		//Update the transform; done automatically when SignalChange is called
		void UpdateTransform();

		//Update internal data
		void UpdateTemp(unsigned int frame_idx);

		//Model pointer
		Model* m_model;
		
		//Translation of mesh node
		DirectX::XMVECTOR m_position;

		//Rotation as quaternion
		DirectX::XMVECTOR m_rotation;

		//Scale
		DirectX::XMVECTOR m_scale;

		//Transformation
		DirectX::XMMATRIX m_transform;

	};

} /* wr */
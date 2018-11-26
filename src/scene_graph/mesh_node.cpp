#include "mesh_node.hpp"

namespace wr 
{

	MeshNode::MeshNode(Model* model)
		: Node(), m_model(model), m_position{ 0, 0, 0, 1 },
		m_rotation(DirectX::XMQuaternionRotationRollPitchYawFromVector({})), m_scale{ 1, 1, 1, 0 }
	{
	}

	void MeshNode::SetRotation(DirectX::XMVECTOR roll_pitch_yaw_deg) {
		float to_deg = 3.1415926435f / 180.f;
		DirectX::XMVECTOR rotation = DirectX::XMVectorMultiply(roll_pitch_yaw_deg, { to_deg, to_deg, to_deg });
		m_rotation = DirectX::XMQuaternionRotationRollPitchYawFromVector(rotation);
		SignalChange();
	}

	void MeshNode::SetPosition(DirectX::XMVECTOR position) {
		m_position = position;
		SignalChange();
	}

	void MeshNode::SetScale(DirectX::XMVECTOR scale) {
		m_scale = scale;
		SignalChange();
	}

	void MeshNode::SetTransform(DirectX::XMVECTOR position, DirectX::XMVECTOR rotation_deg, DirectX::XMVECTOR scale) {
		SetPosition(position);
		SetRotation(rotation_deg);
		SetScale(scale);
	}

	void MeshNode::UpdateTransform() {
		DirectX::XMMATRIX translation_mat = DirectX::XMMatrixTranslationFromVector(m_position);
		DirectX::XMMATRIX rotation_mat = DirectX::XMMatrixRotationQuaternion(m_rotation);
		DirectX::XMMATRIX scale_mat = DirectX::XMMatrixScalingFromVector(m_scale);
		m_transform = scale_mat * rotation_mat * translation_mat;
	}

	void MeshNode::UpdateTemp(unsigned int frame_idx) {
		UpdateTransform();
	}

}
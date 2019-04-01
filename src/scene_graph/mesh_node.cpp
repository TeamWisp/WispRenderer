#include "mesh_node.hpp"

namespace wr {

	MeshNode::MeshNode(Model* model) : m_model(model), m_materials()
	{
	}

	void MeshNode::Update(uint32_t frame_idx)
	{
		m_aabb = AABB::FromTransform(m_model->m_box, m_transform);

		SignalUpdate(frame_idx);
	}

	void MeshNode::AddMaterial(MaterialHandle handle)
	{
		m_materials.push_back(handle);
	}

}
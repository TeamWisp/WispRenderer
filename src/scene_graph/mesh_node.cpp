#include "mesh_node.hpp"

namespace wr {

	MeshNode::MeshNode(Model* model) : m_model(model)
	{
		////Only use 1 AABB when there's just 1 mesh (aka mesh = model)
		//if(model->m_meshes.size() != 1)
		//	m_mesh_aabbs.resize(model->m_meshes.size());
	}

	void MeshNode::Update(uint32_t frame_idx)
	{

		m_aabb = AABB::FromTransform(m_model->m_box, m_transform);

		/*for (size_t i = 0, j = m_model->m_meshes.size(); i < j && j != 1; ++i)
		{
			auto &elem = m_model->m_meshes[i];

			m_mesh_aabbs[i] = AABB::FromTransform(elem.first->m_box, m_transform);

		}*/

		SignalUpdate(frame_idx);
	}

}
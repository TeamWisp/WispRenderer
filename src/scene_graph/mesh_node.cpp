#include "mesh_node.hpp"

namespace wr {

	MeshNode::MeshNode(Model* model) : Node(typeid(MeshNode)), m_model(model), m_materials()
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

		CheckMaterialCount();
	}

	std::vector<MaterialHandle>& MeshNode::GetMaterials()
	{
		return m_materials;
	}

	void MeshNode::SetMaterials(std::vector<MaterialHandle> const & materials)
	{
		m_materials = materials;

		CheckMaterialCount();
	}

	void MeshNode::ClearMaterials()
	{
		m_materials.clear();
	}

	void MeshNode::CheckMaterialCount() const
	{
		if (m_materials.size() > m_model->m_meshes.size())
		{
			LOGW("A mesh node has more materials than meshes.")
		}
	}

}
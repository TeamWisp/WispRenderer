/*!
 * Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "mesh_node.hpp"

namespace wr {

	MeshNode::MeshNode(Model* model) : Node(typeid(MeshNode)), m_model(model), m_materials(), m_visible(true)
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
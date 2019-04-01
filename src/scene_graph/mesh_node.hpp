#pragma once

#include "scene_graph.hpp"
#include "../util/aabb.hpp"

namespace wr
{

	struct MeshNode : Node
	{
		MeshNode(Model* model);

		void Update(uint32_t frame_idx);
		void AddMaterial(MaterialHandle handle);

		Model* m_model;
		AABB m_aabb;
		std::vector<MaterialHandle> m_materials;
	};

} /* wr */
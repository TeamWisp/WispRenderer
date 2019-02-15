#pragma once

#include "scene_graph.hpp"
#include "../util/aabb.hpp"

namespace wr
{

	struct MeshNode : Node
	{
		Model* m_model;

		AABB m_aabb;
		//std::vector<AABB> m_mesh_aabbs;

		MeshNode(Model* model);

		void Update(uint32_t frame_idx);

	};

} /* wr */
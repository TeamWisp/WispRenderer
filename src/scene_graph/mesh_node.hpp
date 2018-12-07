#pragma once

#include "scene_graph.hpp"

namespace wr
{

	struct MeshNode : Node
	{
		Model* m_model;

		MeshNode(Model* model) : m_model(model) { }
	};

} /* wr */
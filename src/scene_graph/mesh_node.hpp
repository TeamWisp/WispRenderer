#pragma once

#include "scene_graph.hpp"

namespace wr
{
	
	struct MeshNode : Node
	{
		DECL_SUBNODE(MeshNode); // TODO: Should be able to dissalow default constructor.

		MeshNode(Model* model)
			: Node(), m_model(model)
		{
			SUBMODE_CONSTRUCTOR
		}

		Model* m_model;

		ConstantBufferHandle* m_transform_cb;
	};

} /* wr */
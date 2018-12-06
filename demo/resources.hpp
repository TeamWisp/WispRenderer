#pragma once

#include "wisp.hpp"

namespace resources
{

	static std::shared_ptr<wr::ModelPool> model_pool;

	static wr::Model* cube_model;
	static wr::Model* plane_model;
	static wr::Model* test_model;

	void CreateResources(wr::RenderSystem* render_system)
	{
		model_pool = render_system->CreateModelPool(2, 2);

		// Load Cube.
		{
			wr::MeshData<wr::VertexNoTangent> mesh;

			mesh.m_indices = {
				2, 1, 0, 3, 2, 0, 6, 5,
				4, 7, 6, 4, 10, 9, 8, 11,
				10, 8, 14, 13, 12, 15, 14, 12,
				18, 17, 16, 19, 18, 16, 22, 21,
				20, 23, 22, 20
			};

			mesh.m_vertices = {
				{ 1, 1, -1, 1, 1, 0, 0, -1 },
				{ 1, -1, -1, 0, 1, 0, 0, -1 },
				{ -1, -1, -1, 0, 0, 0, 0, -1 },
				{ -1, 1, -1, 1, 0, 0, 0, -1 },

				{ 1, 1, 1, 1, 1, 0, 0, 1 },
				{ -1, 1, 1, 0, 1, 0, 0, 1 },
				{ -1, -1, 1, 0, 0, 0, 0, 1 },
				{ 1, -1, 1, 1, 0, 0, 0, 1 },

				{ 1, 1, -1, 1, 0, 1, 0, 0 },
				{ 1, 1, 1, 1, 1, 1, 0, 0 },
				{ 1, -1, 1, 0, 1, 1, 0, 0 },
				{ 1, -1, -1, 0, 0, 1, 0, 0 },

				{ 1, -1, -1, 1, 0, 0, -1, 0 },
				{ 1, -1, 1, 1, 1, 0, -1, 0 },
				{ -1, -1, 1, 0, 1, 0, -1, 0 },
				{ -1, -1, -1, 0, 0, 0, -1, 0 },

				{ -1, -1, -1, 0, 1, -1, 0, 0 },
				{ -1, -1, 1, 0, 0, -1, 0, 0 },
				{ -1, 1, 1, 1, 0, -1, 0, 0 },
				{ -1, 1, -1, 1, 1, -1, 0, 0 },

				{ 1, 1, 1, 1, 0, 0, 1, 0 },
				{ 1, 1, -1, 1, 1, 0, 1, 0 },
				{ -1, 1, -1, 0, 1, 0, 1, 0 },
				{ -1, 1, 1, 0, 0, 0, 1, 0 },
			};

			cube_model = model_pool->LoadCustom<wr::VertexNoTangent>({ mesh });
		}

		{
			wr::MeshData<wr::VertexNoTangent> mesh;

			mesh.m_indices = {
				2, 1, 0, 3, 2, 0
			};

			mesh.m_vertices = {
				{ 1, 1, 0, 1, 1, 0, 0, -1 },
				{ 1, -1, 0, 0, 1, 0, 0, -1 },
				{ -1, -1, 0, 0, 0, 0, 0, -1 },
				{ -1, 1, 0, 1, 0, 0, 0, -1 },
			};

			plane_model = model_pool->LoadCustom<wr::VertexNoTangent>({ mesh });
		}

		{
			test_model = model_pool->Load<wr::VertexNoTangent>("resources/models/xbot.fbx", wr::ModelType::FBX);
		}
	}
	
}
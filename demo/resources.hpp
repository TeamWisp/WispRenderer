#pragma once

#include "wisp.hpp"

namespace resources
{

	static std::shared_ptr<wr::ModelPool> model_pool;
	static std::shared_ptr<wr::TexturePool> texture_pool;
	static std::shared_ptr<wr::MaterialPool> material_pool;

	static wr::Model* cube_model;
	static wr::Model* plane_model;
	static wr::Model* test_model;
	static wr::MaterialHandle rusty_metal_material;
	static wr::MaterialHandle rock_material;

	static wr::Model* robot_model;
	static wr::Model* pica_scene;
	static wr::MaterialHandle pica_scene_material;

	void CreateResources(wr::RenderSystem* render_system)
	{
		texture_pool = render_system->CreateTexturePool(8, 20);
		material_pool = render_system->CreateMaterialPool(8);

		// Load Texture.
		wr::TextureHandle rusty_metal_albedo = texture_pool->Load("resources/materials/rusty_metal/rusty_metal_albedo.png", false, true);
		wr::TextureHandle rusty_metal_normal = texture_pool->Load("resources/materials/rusty_metal/rusty_metal_normal.png", false, true);
		wr::TextureHandle rusty_metal_roughness = texture_pool->Load("resources/materials/rusty_metal/rusty_metal_roughness.png", false, true);
		wr::TextureHandle rusty_metal_metallic = texture_pool->Load("resources/materials/rusty_metal/rusty_metal_metallic.png", false, true);

		// Create Material
		rusty_metal_material = material_pool->Create();

		wr::Material* rusty_metal_internal = material_pool->GetMaterial(rusty_metal_material.m_id);

		rusty_metal_internal->SetAlbedo(rusty_metal_albedo);
		rusty_metal_internal->SetNormal(rusty_metal_normal);
		rusty_metal_internal->SetRoughness(rusty_metal_roughness);
		rusty_metal_internal->SetMetallic(rusty_metal_metallic);

		// Load Texture.
		wr::TextureHandle rock_albedo = texture_pool->Load("resources/materials/rock/rock_albedo.png", false, true);
		wr::TextureHandle rock_normal = texture_pool->Load("resources/materials/rock/rock_normal.png", false, true);
		wr::TextureHandle rock_roughness = texture_pool->Load("resources/materials/rock/rock_roughness.png", false, true);
		wr::TextureHandle rock_metallic = texture_pool->Load("resources/materials/rock/rock_metallic.png", false, true);

		// Create Material
		rock_material = material_pool->Create();

		wr::Material* rock_material_internal = material_pool->GetMaterial(rock_material.m_id);

		rock_material_internal->SetAlbedo(rock_albedo);
		rock_material_internal->SetNormal(rock_normal);
		rock_material_internal->SetRoughness(rock_roughness);
		rock_material_internal->SetMetallic(rock_metallic);

		// Load Texture.
		wr::TextureHandle pica_albedo = texture_pool->Load("resources/materials/floor/albedo.png", false, true);
		wr::TextureHandle pica_normal = texture_pool->Load("resources/materials/floor/normal.png", false, true);
		wr::TextureHandle pica_roughness = texture_pool->Load("resources/materials/floor/roughness.png", false, true);
		wr::TextureHandle pica_metallic = texture_pool->Load("resources/materials/floor/metallic.png", false, true);

		// Create Material
		pica_scene_material = material_pool->Create();

		wr::Material* pica_scene_material_internal = material_pool->GetMaterial(pica_scene_material.m_id);

		pica_scene_material_internal->SetAlbedo(pica_albedo);
		pica_scene_material_internal->SetNormal(pica_normal);
		pica_scene_material_internal->SetRoughness(pica_roughness);
		pica_scene_material_internal->SetMetallic(pica_metallic);
	
		model_pool = render_system->CreateModelPool(16, 16);

		// Load Cube.
		{
			wr::MeshData<wr::Vertex> mesh;

			mesh.m_indices = {
				2, 1, 0, 3, 2, 0, 6, 5,
				4, 7, 6, 4, 10, 9, 8, 11,
				10, 8, 14, 13, 12, 15, 14, 12,
				18, 17, 16, 19, 18, 16, 22, 21,
				20, 23, 22, 20
			};

			mesh.m_vertices = {
				{ 1, 1, -1,		1, 1,		0, 0, -1,		0, 0, 0,	0, 0, 0 },
				{ 1, -1, -1,	0, 1,		0, 0, -1,		0, 0, 0,	0, 0, 0  },
				{ -1, -1, -1,	0, 0,		0, 0, -1,		0, 0, 0,	0, 0, 0  },
				{ -1, 1, -1,	1, 0,		0, 0, -1,		0, 0, 0,	0, 0, 0  },

				{ 1, 1, 1,		1, 1,		0, 0, 1,		0, 0, 0,	0, 0, 0  },
				{ -1, 1, 1,		0, 1,		0, 0, 1,		0, 0, 0,	0, 0, 0  },
				{ -1, -1, 1,	0, 0,		0, 0, 1,		0, 0, 0,	0, 0, 0  },
				{ 1, -1, 1,		1, 0,		0, 0, 1,		0, 0, 0,	0, 0, 0  },

				{ 1, 1, -1,		1, 0,		1, 0, 0,		0, 0, 0,	0, 0, 0  },
				{ 1, 1, 1,		1, 1,		1, 0, 0,		0, 0, 0,	0, 0, 0  },
				{ 1, -1, 1,		0, 1,		1, 0, 0,		0, 0, 0,	0, 0, 0  },
				{ 1, -1, -1,	0, 0,		1, 0, 0,		0, 0, 0,	0, 0, 0  },

				{ 1, -1, -1,	1, 0,		0, -1, 0,		0, 0, 0,	0, 0, 0  },
				{ 1, -1, 1,		1, 1,		0, -1, 0,		0, 0, 0,	0, 0, 0  },
				{ -1, -1, 1,	0, 1,		0, -1, 0,		0, 0, 0,	0, 0, 0  },
				{ -1, -1, -1,	0, 0,		0, -1, 0,		0, 0, 0,	0, 0, 0  },

				{ -1, -1, -1,	0, 1,		-1, 0, 0,		0, 0, 0,	0, 0, 0  },
				{ -1, -1, 1,	0, 0,		-1, 0, 0,		0, 0, 0,	0, 0, 0  },
				{ -1, 1, 1,		1, 0,		-1, 0, 0,		0, 0, 0,	0, 0, 0  },
				{ -1, 1, -1,	1, 1,		-1, 0, 0,		0, 0, 0,	0, 0, 0  },

				{ 1, 1, 1,		1, 0,		0, 1, 0,		0, 0, 0,	0, 0, 0  },
				{ 1, 1, -1,		1, 1,		0, 1, 0,		0, 0, 0,	0, 0, 0  },
				{ -1, 1, -1,	0, 1,		0, 1, 0,		0, 0, 0,	0, 0, 0  },
				{ -1, 1, 1,		0, 0,		0, 1, 0,		0, 0, 0,	0, 0, 0  },
			};

			cube_model = model_pool->LoadCustom<wr::Vertex>({ mesh });

			for (auto& m : cube_model->m_meshes)
			{
				m.second = &rock_material;
			}
		}

		{
			wr::MeshData<wr::Vertex> mesh;

			mesh.m_indices = {
				2, 1, 0, 3, 2, 0
			};

			mesh.m_vertices = {
				//POS				UV			NORMAL				TANGENT			BINORMAL
				{  1,  1,  0,		1, 1,		0, 0, -1,			0, 0, 1,		0, 1, 0},
				{  1, -1,  0,		1, 0,		0, 0, -1,			0, 0, 1,		0, 1, 0},
				{ -1, -1,  0,		0, 0,		0, 0, -1,			0, 0, 1,		0, 1, 0},
				{ -1,  1,  0,		0, 1,		0, 0, -1,			0, 0, 1,		0, 1, 0},
			};

			plane_model = model_pool->LoadCustom<wr::Vertex>({ mesh });

			for (auto& m : plane_model->m_meshes)
			{
				m.second = &rock_material;
			}
		}

		{
			test_model = model_pool->Load<wr::Vertex>(material_pool.get(), texture_pool.get(), "resources/models/xbot.fbx");
		
			for (auto& m : test_model->m_meshes)
			{
				m.second = &rusty_metal_material;
			}
		}


		{
			//robot_model = model_pool->Load<wr::Vertex>(material_pool.get(), texture_pool.get(), "resources/models/robot.fbx");

			//for (auto& m : robot_model->m_meshes)
			{
				//m.second = &rusty_metal_material;
			}
		}
		{
			pica_scene = model_pool->Load<wr::Vertex>(material_pool.get(), texture_pool.get(), "resources/models/pica_scene.fbx");

			for (auto& m : pica_scene->m_meshes)
			{
				m.second = &pica_scene_material;
			}
		}
	}

}

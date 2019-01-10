#pragma once

#include "wisp.hpp"

namespace resources
{

	static std::shared_ptr<wr::ModelPool> model_pool;
	static std::shared_ptr<wr::TexturePool> texture_pool;
	static std::shared_ptr<wr::MaterialPool> material_pool;

	static wr::Model* cube_model;
	static wr::Model* plane_model;
	static wr::Model* light_model;
	static wr::Model* test_model;
	static wr::MaterialHandle rusty_metal_material;
	static wr::MaterialHandle rock_material;
	static wr::MaterialHandle light_material;

	void CreateResources(wr::RenderSystem* render_system)
	{
		texture_pool = render_system->CreateTexturePool(16, 14);
		material_pool = render_system->CreateMaterialPool(8);

		// Load Texture.
		wr::TextureHandle black = texture_pool->Load("resources/materials/black.png", false, true);

		wr::TextureHandle metal_splotchy_albedo = texture_pool->Load("resources/materials/metal-splotchy-albedo.png", false, true);
		wr::TextureHandle metal_splotchy_normal = texture_pool->Load("resources/materials/metal-splotchy-normal-dx.png", false, true);
		wr::TextureHandle metal_splotchy_roughness = texture_pool->Load("resources/materials/metal-splotchy-rough.png", false, true);
		wr::TextureHandle metal_splotchy_metallic = texture_pool->Load("resources/materials/metal-splotchy-metal.png", false, true);

		wr::TextureHandle bamboo_albedo = texture_pool->Load("resources/materials/bamboo/bamboo-wood-semigloss-albedo.png", false, true);
		wr::TextureHandle bamboo_normal = texture_pool->Load("resources/materials/bamboo/bamboo-wood-semigloss-normal.png", false, true);
		wr::TextureHandle bamboo_roughness = texture_pool->Load("resources/materials/bamboo/bamboo-wood-semigloss-roughness.png", false, true);
		wr::TextureHandle bamboo_metallic = texture_pool->Load("resources/materials/bamboo/bamboo-wood-semigloss-metal.png", false, true);

		// Create Material
		light_material = material_pool->Create();

		wr::MaterialPool::SetMaterialHandleAlbedo(light_material, black);
		wr::MaterialPool::SetMaterialHandleNormal(light_material, black);
		wr::MaterialPool::SetMaterialHandleRoughness(light_material, black);
		wr::MaterialPool::SetMaterialHandleMetallic(light_material, black);
		
		// Create Material
		rusty_metal_material = material_pool->Create();

		wr::MaterialPool::SetMaterialHandleAlbedo(rusty_metal_material, metal_splotchy_albedo);
		wr::MaterialPool::SetMaterialHandleNormal(rusty_metal_material, metal_splotchy_normal);
		wr::MaterialPool::SetMaterialHandleRoughness(rusty_metal_material, metal_splotchy_roughness);
		wr::MaterialPool::SetMaterialHandleMetallic(rusty_metal_material, metal_splotchy_metallic);

		// Create Material
		rock_material = material_pool->Create();

		wr::MaterialPool::SetMaterialHandleAlbedo(rock_material, bamboo_albedo);
		wr::MaterialPool::SetMaterialHandleNormal(rock_material, bamboo_normal);
		wr::MaterialPool::SetMaterialHandleRoughness(rock_material, bamboo_roughness);
		wr::MaterialPool::SetMaterialHandleMetallic(rock_material, bamboo_metallic);
			
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

			light_model = model_pool->LoadCustom<wr::Vertex>({ mesh });
			plane_model = model_pool->LoadCustom<wr::Vertex>({ mesh });
		}


		{
			test_model = model_pool->Load<wr::Vertex>(material_pool.get(), texture_pool.get(), "resources/models/xbot.fbx");
		}
	}

}

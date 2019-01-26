#pragma once

#include "wisp.hpp"
#include "d3d12/d3d12_structs.hpp"

namespace resources
{

	static std::shared_ptr<wr::ModelPool> model_pool;
	static std::shared_ptr<wr::TexturePool> texture_pool;
	static std::shared_ptr<wr::MaterialPool> material_pool;

	static wr::Model* cube_model;
	static wr::Model* plane_model;
	static wr::Model* light_model;
	static wr::Model* human_model;
	static wr::Model* test_model;
	static wr::Model* sphere_model;
	static wr::Model* material_knot_bamboo;
	static wr::Model* material_knot_plastic;
	static wr::Model* material_knot_metal;
	static wr::MaterialHandle rusty_metal_material;
	static wr::MaterialHandle bamboo_material;
	static wr::MaterialHandle plastic_material;
	static wr::MaterialHandle metal_material;

	static wr::MaterialHandle light_material;
	static wr::MaterialHandle mirror_material;
	static wr::TextureHandle equirectangular_environment_map;

	void CreateResources(wr::RenderSystem* render_system)
	{
		texture_pool = render_system->CreateTexturePool(16, 24);
		material_pool = render_system->CreateMaterialPool(20);

		// Load Texture.
		wr::TextureHandle white = texture_pool->Load("resources/materials/white.png", false, true);
		wr::TextureHandle black = texture_pool->Load("resources/materials/black.png", false, true);
		wr::TextureHandle flat_normal = texture_pool->Load("resources/materials/flat_normal.png", false, true);

		wr::TextureHandle metal_splotchy_albedo = texture_pool->Load("resources/materials/metal-splotchy-albedo.png", true, true);
		wr::TextureHandle metal_splotchy_normal = texture_pool->Load("resources/materials/metal-splotchy-normal-dx.png", false, true);
		wr::TextureHandle metal_splotchy_roughness = texture_pool->Load("resources/materials/metal-splotchy-rough.png", false, true);
		wr::TextureHandle metal_splotchy_metallic = texture_pool->Load("resources/materials/metal-splotchy-metal.png", false, true);

		wr::TextureHandle bamboo_albedo = texture_pool->Load("resources/materials/bamboo/bamboo-wood-semigloss-albedo.png", true, true);
		wr::TextureHandle bamboo_normal = texture_pool->Load("resources/materials/bamboo/bamboo-wood-semigloss-normal.png", false, true);
		wr::TextureHandle bamboo_roughness = texture_pool->Load("resources/materials/bamboo/bamboo-wood-semigloss-roughness.png", false, true);
		wr::TextureHandle bamboo_metallic = texture_pool->Load("resources/materials/bamboo/bamboo-wood-semigloss-metal.png", false, true);

		wr::TextureHandle plastic_albedo = texture_pool->Load("resources/materials/scruffed_plastic/albedo.png", true, true);
		wr::TextureHandle plastic_normal = texture_pool->Load("resources/materials/scruffed_plastic/normal.png", false, true);
		wr::TextureHandle plastic_roughness = texture_pool->Load("resources/materials/scruffed_plastic/roughness.png", false, true);
		wr::TextureHandle plastic_metallic = texture_pool->Load("resources/materials/scruffed_plastic/metallic.png", false, true);

		wr::TextureHandle metal_albedo = texture_pool->Load("resources/materials/greasy_pan/albedo.png", true, true);
		wr::TextureHandle metal_normal = texture_pool->Load("resources/materials/greasy_pan/normal.png", false, true);
		wr::TextureHandle metal_roughness = texture_pool->Load("resources/materials/greasy_pan/roughness.png", false, true);
		wr::TextureHandle metal_metallic = texture_pool->Load("resources/materials/greasy_pan/metallic.png", false, true);

		equirectangular_environment_map = texture_pool->Load("resources/materials/Circus_Backstage_3k.hdr", false, false);

		// Create Material
		mirror_material = material_pool->Create();

		wr::Material* mirror_internal = material_pool->GetMaterial(mirror_material.m_id);

		mirror_internal->SetAlbedo(white);
		mirror_internal->SetNormal(flat_normal);
		mirror_internal->SetRoughness(black);
		mirror_internal->SetMetallic(white);

		{
			// Create Material
			rusty_metal_material = material_pool->Create();

			wr::Material* rusty_metal_internal = material_pool->GetMaterial(rusty_metal_material.m_id);

			rusty_metal_internal->SetAlbedo(metal_splotchy_albedo);
			rusty_metal_internal->SetNormal(metal_splotchy_normal);
			rusty_metal_internal->SetRoughness(metal_splotchy_roughness);
			rusty_metal_internal->SetMetallic(metal_splotchy_metallic);

			// Create Material
			bamboo_material = material_pool->Create();

			wr::Material* bamboo_material_internal = material_pool->GetMaterial(bamboo_material.m_id);

			bamboo_material_internal->SetAlbedo(bamboo_albedo);
			bamboo_material_internal->SetNormal(bamboo_normal);
			bamboo_material_internal->SetRoughness(bamboo_roughness);
			bamboo_material_internal->SetMetallic(bamboo_metallic);

			// Create Material
			plastic_material = material_pool->Create();

			wr::Material* plastic_material_internal = material_pool->GetMaterial(plastic_material.m_id);

			plastic_material_internal->SetAlbedo(plastic_albedo);
			plastic_material_internal->SetNormal(plastic_normal);
			plastic_material_internal->SetRoughness(plastic_roughness);
			plastic_material_internal->SetMetallic(plastic_metallic);

			// Create Material
			metal_material = material_pool->Create();

			wr::Material* metal_material_internal = material_pool->GetMaterial(metal_material.m_id);

			metal_material_internal->SetAlbedo(metal_albedo);
			metal_material_internal->SetNormal(metal_normal);
			metal_material_internal->SetRoughness(metal_roughness);
			metal_material_internal->SetMetallic(metal_metallic);
		
		}

		model_pool = render_system->CreateModelPool(300, 300);

		{
			wr::MeshData<wr::VertexColor> mesh;

				mesh.m_indices = {
					2, 1, 0, 3, 2, 0
				};

				mesh.m_vertices = {
					//POS                UV            NORMAL                TANGENT            BINORMAL	COLOR
					{  1,  1,  0,        1, 1,        0, 0, -1,            0, 0, 1,        0, 1, 0,			0, 0, 0 },
					{  1, -1,  0,        1, 0,        0, 0, -1,            0, 0, 1,        0, 1, 0,			0, 0, 0 },
					{ -1, -1,  0,        0, 0,        0, 0, -1,            0, 0, 1,        0, 1, 0,			0, 0, 0 },
					{ -1,  1,  0,        0, 1,        0, 0, -1,            0, 0, 1,        0, 1, 0,			0, 0, 0 },
				};

				//plane_model = model_pool->LoadCustom<wr::Vertex>({ mesh });
				plane_model = model_pool->Load<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/plane.fbx");

			//plane_model = model_pool->LoadCustom<wr::VertexColor>({ mesh });
			light_model = plane_model;

				for (auto& m : plane_model->m_meshes)
				{
					m.second = &bamboo_material;
				}
		}

		{
			{
				test_model = model_pool->LoadWithMaterials<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/materials/world.fbx");
				sphere_model = model_pool->Load<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/sphere.fbx");

				for (auto& m : test_model->m_meshes)
				{
					//if (!m.second)
					//m.second = &rusty_metal_material;
				}

				for (auto& m : sphere_model->m_meshes)
				{
					m.second = &mirror_material;
				}
			}

			{
				//material_knot_bamboo = model_pool->Load<wr::Vertex>(material_pool.get(), texture_pool.get(), "resources/models/material_ball.fbx");

				human_model = model_pool->Load<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/human.fbx");

				for (auto& m : human_model->m_meshes)
				{
					m.second = &plastic_material;
				}

				//for (auto& m : material_knot_bamboo->m_meshes)
				{
				//	m.second = &bamboo_material;
				}
			}

			{
				material_knot_plastic = new wr::Model();

				/*for (auto& m : material_knot_bamboo->m_meshes)
				{
					material_knot_plastic->m_meshes.push_back(std::make_pair(m.first, &plastic_material));
					material_knot_plastic->m_box[0] = material_knot_bamboo->m_box[0];
					material_knot_plastic->m_box[1] = material_knot_bamboo->m_box[1];
					material_knot_plastic->m_box[2] = material_knot_bamboo->m_box[2];
					material_knot_plastic->m_box[3] = material_knot_bamboo->m_box[3];
					material_knot_plastic->m_box[4] = material_knot_bamboo->m_box[4];
					material_knot_plastic->m_box[5] = material_knot_bamboo->m_box[5];
					material_knot_plastic->m_model_name = material_knot_bamboo->m_model_name;
					material_knot_plastic->m_model_pool = material_knot_bamboo->m_model_pool;
				}*/
			}

			{
				material_knot_metal = new wr::Model();

				/*for (auto& m : material_knot_bamboo->m_meshes)
				{
					material_knot_metal->m_meshes.push_back(std::make_pair(m.first, &metal_material));
					material_knot_metal->m_box[0] = material_knot_bamboo->m_box[0];
					material_knot_metal->m_box[1] = material_knot_bamboo->m_box[1];
					material_knot_metal->m_box[2] = material_knot_bamboo->m_box[2];
					material_knot_metal->m_box[3] = material_knot_bamboo->m_box[3];
					material_knot_metal->m_box[4] = material_knot_bamboo->m_box[4];
					material_knot_metal->m_box[5] = material_knot_bamboo->m_box[5];
					material_knot_metal->m_model_name = material_knot_bamboo->m_model_name;
					material_knot_metal->m_model_pool = material_knot_bamboo->m_model_pool;
				}*/
			}

		}
	}

#pragma region REPLACE_LOADING_WITH_THIS
		// Once the ray tracing implementation supports multiple model pools
		// Replace the loading code above with this one.

		//// Load Cube.
		//{
		//	cube_model = render_system->GetSimpleShape(wr::RenderSystem::SimpleShapes::CUBE);

		//	for (auto& m : cube_model->m_meshes)
		//	{
		//		m.second = &rock_material;
		//	}
		//}

		//{
		//	plane_model = render_system->GetSimpleShape(wr::RenderSystem::SimpleShapes::PLANE);

		//	light_model = plane_model;

		//	for (auto& m : plane_model->m_meshes)
		//	{
		//		m.second = &rock_material;
		//	}
		//}


		//{
		//	test_model = model_pool->Load<wr::Vertex>(material_pool.get(), texture_pool.get(), "resources/models/xbot.fbx");

		//	for (auto& m : test_model->m_meshes)
		//	{
		//		m.second = &rusty_metal_material;
		//	}
		//}
#pragma endregion

}
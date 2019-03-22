
#pragma once

#include "wisp.hpp"
#include "window.hpp"
#include "scene_graph/scene_graph.hpp"
#include "imgui/imgui.hpp"
#include "debug_camera.hpp"

namespace emibl_scene
{
	namespace resources
	{
		//////////////////////////
		// RESOURCES
		//////////////////////////

		static std::shared_ptr<wr::ModelPool> model_pool;
		static std::shared_ptr<wr::TexturePool> texture_pool;
		static std::shared_ptr<wr::MaterialPool> material_pool;

		static wr::Model* cube_model;
		static wr::Model* plane_model;
		static wr::Model* test_model;
		static wr::Model* material_knot_mahogfloor;
		static wr::Model* material_knot_red_black_pattern;
		static wr::Model* material_knot_metal;
		static wr::Model* material_knot_brick_tiles;
		static wr::Model* material_knot_leather;
		static wr::Model* material_knot_blue_tiles;
		static wr::Model* material_knot_gold;
		static wr::Model* material_knot_marble;
		static wr::Model* material_knot_floreal_tiles;
		static wr::Model* material_knot_bw_tiles;

		static wr::MaterialHandle rusty_metal_material;
		static wr::MaterialHandle mahogfloor_material;
		static wr::MaterialHandle red_black_pattern_material;
		static wr::MaterialHandle metal_material;
		static wr::MaterialHandle brick_tiles_material;
		static wr::MaterialHandle leather_material;
		static wr::MaterialHandle blue_tiles_material;
		static wr::MaterialHandle gold_material;
		static wr::MaterialHandle marble_material;
		static wr::MaterialHandle floreal_tiles_material;
		static wr::MaterialHandle bw_tiles_material;

		static wr::TextureHandle equirectangular_environment_map;

		static wr::TextureHandle flat_normal;

		void CreateResources(wr::RenderSystem* render_system)
		{
			texture_pool = render_system->CreateTexturePool();
			material_pool = render_system->CreateMaterialPool(256);

			// Load Texture.
			wr::TextureHandle white = texture_pool->Load("resources/materials/white.png", false, true);
			wr::TextureHandle black = texture_pool->Load("resources/materials/black.png", false, true);
			flat_normal = texture_pool->Load("resources/materials/flat_normal.png", false, true);

			wr::TextureHandle metal_splotchy_albedo = texture_pool->Load("resources/materials/metal-splotchy-albedo.png", true, true);
			wr::TextureHandle metal_splotchy_normal = texture_pool->Load("resources/materials/metal-splotchy-normal-dx.png", false, true);
			wr::TextureHandle metal_splotchy_roughness = texture_pool->Load("resources/materials/metal-splotchy-rough.png", false, true);
			wr::TextureHandle metal_splotchy_metallic = texture_pool->Load("resources/materials/metal-splotchy-metal.png", false, true);

			wr::TextureHandle mahogfloor_albedo = texture_pool->Load("resources/materials/mahogfloor/albedo.png", true, true);
			wr::TextureHandle mahogfloor_normal = texture_pool->Load("resources/materials/mahogfloor/normal.png", false, true);
			wr::TextureHandle mahogfloor_roughness = texture_pool->Load("resources/materials/mahogfloor/roughness.png", false, true);
			wr::TextureHandle mahogfloor_metallic = texture_pool->Load("resources/materials/mahogfloor/metallic.png", false, true);

			wr::TextureHandle red_black_albedo = texture_pool->Load("resources/materials/red_black_pattern_leather/albedo.png", true, true);
			wr::TextureHandle red_black_normal = texture_pool->Load("resources/materials/red_black_pattern_leather/normal.png", false, true);
			wr::TextureHandle red_black_roughness = texture_pool->Load("resources/materials/red_black_pattern_leather/roughness.png", false, true);
			wr::TextureHandle red_black_metallic = texture_pool->Load("resources/materials/red_black_pattern_leather/metallic.png", false, true);

			wr::TextureHandle metal_albedo = texture_pool->Load("resources/materials/greasy_pan/albedo.png", true, true);
			wr::TextureHandle metal_normal = texture_pool->Load("resources/materials/greasy_pan/normal.png", false, true);
			wr::TextureHandle metal_roughness = texture_pool->Load("resources/materials/greasy_pan/roughness.png", false, true);
			wr::TextureHandle metal_metallic = texture_pool->Load("resources/materials/greasy_pan/metallic.png", false, true);

			wr::TextureHandle brick_tiles_albedo = texture_pool->Load("resources/materials/brick_tiles/albedo.png", true, true);
			wr::TextureHandle brick_tiles_normal = texture_pool->Load("resources/materials/brick_tiles/normal.png", false, true);
			wr::TextureHandle brick_tiles_roughness = texture_pool->Load("resources/materials/brick_tiles/roughness.png", false, true);
			wr::TextureHandle brick_tiles_metallic = texture_pool->Load("resources/materials/brick_tiles/metallic.png", false, true);

			wr::TextureHandle leather_albedo = texture_pool->Load("resources/materials/leather_with_metal/albedo.png", true, true);
			wr::TextureHandle leather_normal = texture_pool->Load("resources/materials/leather_with_metal/normal.png", false, true);
			wr::TextureHandle leather_roughness = texture_pool->Load("resources/materials/leather_with_metal/roughness.png", false, true);
			wr::TextureHandle leather_metallic = texture_pool->Load("resources/materials/leather_with_metal/metallic.png", false, true);

			wr::TextureHandle blue_tiles_albedo = texture_pool->Load("resources/materials/blue_tiles/albedo.png", true, true);
			wr::TextureHandle blue_tiles_normal = texture_pool->Load("resources/materials/blue_tiles/normal.png", false, true);
			wr::TextureHandle blue_tiles_roughness = texture_pool->Load("resources/materials/blue_tiles/roughness.png", false, true);
			wr::TextureHandle blue_tiles_metallic = texture_pool->Load("resources/materials/blue_tiles/metallic.png", false, true);

			wr::TextureHandle gold_albedo = texture_pool->Load("resources/materials/gold_scuffed/albedo.png", true, true);
			wr::TextureHandle gold_normal = texture_pool->Load("resources/materials/gold_scuffed/normal.png", false, true);
			wr::TextureHandle gold_roughness = texture_pool->Load("resources/materials/gold_scuffed/roughness.png", false, true);
			wr::TextureHandle gold_metallic = texture_pool->Load("resources/materials/gold_scuffed/metallic.png", false, true);

			wr::TextureHandle marble_albedo = texture_pool->Load("resources/materials/marble_speckled/albedo.png", true, true);
			wr::TextureHandle marble_normal = texture_pool->Load("resources/materials/marble_speckled/normal.png", false, true);
			wr::TextureHandle marble_roughness = texture_pool->Load("resources/materials/marble_speckled/roughness.png", false, true);
			wr::TextureHandle marble_metallic = texture_pool->Load("resources/materials/marble_speckled/metallic.png", false, true);


			wr::TextureHandle floreal_tiles_albedo = texture_pool->Load("resources/materials/floreal_tiles/albedo.png", true, true);
			wr::TextureHandle floreal_tiles_normal = texture_pool->Load("resources/materials/floreal_tiles/normal.png", false, true);
			wr::TextureHandle floreal_tiles_roughness = texture_pool->Load("resources/materials/floreal_tiles/roughness.png", false, true);
			wr::TextureHandle floreal_tiles_metallic = texture_pool->Load("resources/materials/floreal_tiles/metallic.png", false, true);

			wr::TextureHandle bw_tiles_albedo = texture_pool->Load("resources/materials/bw_tiles_gold_lining/albedo.png", true, true);
			wr::TextureHandle bw_tiles_normal = texture_pool->Load("resources/materials/bw_tiles_gold_lining/normal.png", false, true);
			wr::TextureHandle bw_tiles_roughness = texture_pool->Load("resources/materials/bw_tiles_gold_lining/roughness.png", false, true);
			wr::TextureHandle bw_tiles_metallic = texture_pool->Load("resources/materials/bw_tiles_gold_lining/metallic.png", false, true);

			equirectangular_environment_map = texture_pool->Load("resources/materials/artist_workshop_4k.hdr", false, false);

			{
				// Create Material
				rusty_metal_material = material_pool->Create();

				wr::Material* rusty_metal_internal = material_pool->GetMaterial(rusty_metal_material.m_id);

				rusty_metal_internal->SetAlbedo(metal_splotchy_albedo);
				rusty_metal_internal->SetNormal(metal_splotchy_normal);
				rusty_metal_internal->SetRoughness(metal_splotchy_roughness);
				rusty_metal_internal->SetMetallic(metal_splotchy_metallic);

				// Create Material
				mahogfloor_material = material_pool->Create();

				wr::Material* mahogfloor_material_internal = material_pool->GetMaterial(mahogfloor_material.m_id);

				mahogfloor_material_internal->SetAlbedo(mahogfloor_albedo);
				mahogfloor_material_internal->SetNormal(mahogfloor_normal);
				mahogfloor_material_internal->SetRoughness(mahogfloor_roughness);
				mahogfloor_material_internal->SetMetallic(mahogfloor_metallic);

				// Create Material
				red_black_pattern_material = material_pool->Create();

				wr::Material* red_black_pattern_internal = material_pool->GetMaterial(red_black_pattern_material.m_id);

				red_black_pattern_internal->SetAlbedo(red_black_albedo);
				red_black_pattern_internal->SetNormal(red_black_normal);
				red_black_pattern_internal->SetRoughness(red_black_roughness);
				red_black_pattern_internal->SetMetallic(red_black_metallic);

				// Create Material
				metal_material = material_pool->Create();

				wr::Material* metal_material_internal = material_pool->GetMaterial(metal_material.m_id);

				metal_material_internal->SetAlbedo(metal_albedo);
				metal_material_internal->SetNormal(metal_normal);
				metal_material_internal->SetRoughness(metal_roughness);
				metal_material_internal->SetMetallic(metal_metallic);

				brick_tiles_material = material_pool->Create();

				wr::Material* brick_tiles_mat_internal = material_pool->GetMaterial(brick_tiles_material.m_id);

				brick_tiles_mat_internal->SetAlbedo(brick_tiles_albedo);
				brick_tiles_mat_internal->SetNormal(brick_tiles_normal);
				brick_tiles_mat_internal->SetRoughness(brick_tiles_roughness);
				brick_tiles_mat_internal->SetMetallic(brick_tiles_metallic);

				leather_material = material_pool->Create();

				wr::Material* leather_material_internal = material_pool->GetMaterial(leather_material.m_id);

				leather_material_internal->SetAlbedo(leather_albedo);
				leather_material_internal->SetNormal(leather_normal);
				leather_material_internal->SetRoughness(leather_roughness);
				leather_material_internal->SetMetallic(leather_metallic);

				blue_tiles_material = material_pool->Create();

				wr::Material* blue_tiles_material_internal = material_pool->GetMaterial(blue_tiles_material.m_id);

				blue_tiles_material_internal->SetAlbedo(blue_tiles_albedo);
				blue_tiles_material_internal->SetNormal(blue_tiles_normal);
				blue_tiles_material_internal->SetRoughness(blue_tiles_roughness);
				blue_tiles_material_internal->SetMetallic(blue_tiles_metallic);

				gold_material = material_pool->Create();

				wr::Material* gold_material_internal = material_pool->GetMaterial(gold_material.m_id);

				gold_material_internal->SetAlbedo(gold_albedo);
				gold_material_internal->SetNormal(gold_normal);
				gold_material_internal->SetRoughness(gold_roughness);
				gold_material_internal->SetMetallic(gold_metallic);

				marble_material = material_pool->Create();

				wr::Material* marble_material_internal = material_pool->GetMaterial(marble_material.m_id);

				marble_material_internal->SetAlbedo(marble_albedo);
				marble_material_internal->SetNormal(marble_normal);
				marble_material_internal->SetRoughness(marble_roughness);
				marble_material_internal->SetMetallic(marble_metallic);

				floreal_tiles_material = material_pool->Create();

				wr::Material* rubber_material_internal = material_pool->GetMaterial(floreal_tiles_material.m_id);

				rubber_material_internal->SetAlbedo(floreal_tiles_albedo);
				rubber_material_internal->SetNormal(floreal_tiles_normal);
				rubber_material_internal->SetRoughness(floreal_tiles_roughness);
				rubber_material_internal->SetMetallic(floreal_tiles_metallic);

				bw_tiles_material = material_pool->Create();

				wr::Material* scorched_wood_material_internal = material_pool->GetMaterial(bw_tiles_material.m_id);

				scorched_wood_material_internal->SetAlbedo(bw_tiles_albedo);
				scorched_wood_material_internal->SetNormal(bw_tiles_normal);
				scorched_wood_material_internal->SetRoughness(bw_tiles_roughness);
				scorched_wood_material_internal->SetMetallic(bw_tiles_metallic);
			}

			model_pool = render_system->CreateModelPool(64_mb, 64_mb);

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

				plane_model = model_pool->Load<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/plane.fbx");

				for (auto& m : plane_model->m_meshes)
				{
					m.second = mahogfloor_material;
				}
			}

			{
				{
					test_model = model_pool->LoadWithMaterials<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/xbot.fbx");

					//for (auto& m : test_model->m_meshes)
					//{
					//	m.second = &rusty_metal_material;
					//}

				}

				{
					material_knot_mahogfloor = model_pool->Load<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/material_ball.fbx");

					for (auto& m : material_knot_mahogfloor->m_meshes)
					{
						m.second = mahogfloor_material;
					}
				}

				{
					material_knot_red_black_pattern = new wr::Model();

					for (auto& m : material_knot_mahogfloor->m_meshes)
					{
						material_knot_red_black_pattern->m_meshes.push_back(std::make_pair(m.first, red_black_pattern_material));
						material_knot_red_black_pattern->m_box[0] = material_knot_mahogfloor->m_box[0];
						material_knot_red_black_pattern->m_box[1] = material_knot_mahogfloor->m_box[1];
						material_knot_red_black_pattern->m_box[2] = material_knot_mahogfloor->m_box[2];
						material_knot_red_black_pattern->m_box[3] = material_knot_mahogfloor->m_box[3];
						material_knot_red_black_pattern->m_box[4] = material_knot_mahogfloor->m_box[4];
						material_knot_red_black_pattern->m_box[5] = material_knot_mahogfloor->m_box[5];
						material_knot_red_black_pattern->m_model_name = material_knot_mahogfloor->m_model_name;
						material_knot_red_black_pattern->m_model_pool = material_knot_mahogfloor->m_model_pool;
					}
				}

				{
					material_knot_metal = new wr::Model();

					for (auto& m : material_knot_mahogfloor->m_meshes)
					{
						material_knot_metal->m_meshes.push_back(std::make_pair(m.first, metal_material));
						material_knot_metal->m_box[0] = material_knot_mahogfloor->m_box[0];
						material_knot_metal->m_box[1] = material_knot_mahogfloor->m_box[1];
						material_knot_metal->m_box[2] = material_knot_mahogfloor->m_box[2];
						material_knot_metal->m_box[3] = material_knot_mahogfloor->m_box[3];
						material_knot_metal->m_box[4] = material_knot_mahogfloor->m_box[4];
						material_knot_metal->m_box[5] = material_knot_mahogfloor->m_box[5];
						material_knot_metal->m_model_name = material_knot_mahogfloor->m_model_name;
						material_knot_metal->m_model_pool = material_knot_mahogfloor->m_model_pool;
					}
				}
				{
					material_knot_brick_tiles = new wr::Model();

					for (auto& m : material_knot_mahogfloor->m_meshes)
					{
						material_knot_brick_tiles->m_meshes.push_back(std::make_pair(m.first, brick_tiles_material));
						material_knot_brick_tiles->m_box[0] = material_knot_mahogfloor->m_box[0];
						material_knot_brick_tiles->m_box[1] = material_knot_mahogfloor->m_box[1];
						material_knot_brick_tiles->m_box[2] = material_knot_mahogfloor->m_box[2];
						material_knot_brick_tiles->m_box[3] = material_knot_mahogfloor->m_box[3];
						material_knot_brick_tiles->m_box[4] = material_knot_mahogfloor->m_box[4];
						material_knot_brick_tiles->m_box[5] = material_knot_mahogfloor->m_box[5];
						material_knot_brick_tiles->m_model_name = material_knot_mahogfloor->m_model_name;
						material_knot_brick_tiles->m_model_pool = material_knot_mahogfloor->m_model_pool;
					}
				}

				{
					material_knot_leather = new wr::Model();

					for (auto& m : material_knot_mahogfloor->m_meshes)
					{
						material_knot_leather->m_meshes.push_back(std::make_pair(m.first, leather_material));
						material_knot_leather->m_box[0] = material_knot_mahogfloor->m_box[0];
						material_knot_leather->m_box[1] = material_knot_mahogfloor->m_box[1];
						material_knot_leather->m_box[2] = material_knot_mahogfloor->m_box[2];
						material_knot_leather->m_box[3] = material_knot_mahogfloor->m_box[3];
						material_knot_leather->m_box[4] = material_knot_mahogfloor->m_box[4];
						material_knot_leather->m_box[5] = material_knot_mahogfloor->m_box[5];
						material_knot_leather->m_model_name = material_knot_mahogfloor->m_model_name;
						material_knot_leather->m_model_pool = material_knot_mahogfloor->m_model_pool;
					}
				}

				{
					material_knot_blue_tiles = new wr::Model();

					for (auto& m : material_knot_mahogfloor->m_meshes)
					{
						material_knot_blue_tiles->m_meshes.push_back(std::make_pair(m.first, blue_tiles_material));
						material_knot_blue_tiles->m_box[0] = material_knot_mahogfloor->m_box[0];
						material_knot_blue_tiles->m_box[1] = material_knot_mahogfloor->m_box[1];
						material_knot_blue_tiles->m_box[2] = material_knot_mahogfloor->m_box[2];
						material_knot_blue_tiles->m_box[3] = material_knot_mahogfloor->m_box[3];
						material_knot_blue_tiles->m_box[4] = material_knot_mahogfloor->m_box[4];
						material_knot_blue_tiles->m_box[5] = material_knot_mahogfloor->m_box[5];
						material_knot_blue_tiles->m_model_name = material_knot_mahogfloor->m_model_name;
						material_knot_blue_tiles->m_model_pool = material_knot_mahogfloor->m_model_pool;
					}
				}

				{
					material_knot_gold = new wr::Model();

					for (auto& m : material_knot_mahogfloor->m_meshes)
					{
						material_knot_gold->m_meshes.push_back(std::make_pair(m.first, gold_material));
						material_knot_gold->m_box[0] = material_knot_mahogfloor->m_box[0];
						material_knot_gold->m_box[1] = material_knot_mahogfloor->m_box[1];
						material_knot_gold->m_box[2] = material_knot_mahogfloor->m_box[2];
						material_knot_gold->m_box[3] = material_knot_mahogfloor->m_box[3];
						material_knot_gold->m_box[4] = material_knot_mahogfloor->m_box[4];
						material_knot_gold->m_box[5] = material_knot_mahogfloor->m_box[5];
						material_knot_gold->m_model_name = material_knot_mahogfloor->m_model_name;
						material_knot_gold->m_model_pool = material_knot_mahogfloor->m_model_pool;
					}
				}

				{
					material_knot_marble = new wr::Model();

					for (auto& m : material_knot_mahogfloor->m_meshes)
					{
						material_knot_marble->m_meshes.push_back(std::make_pair(m.first, marble_material));
						material_knot_marble->m_box[0] = material_knot_mahogfloor->m_box[0];
						material_knot_marble->m_box[1] = material_knot_mahogfloor->m_box[1];
						material_knot_marble->m_box[2] = material_knot_mahogfloor->m_box[2];
						material_knot_marble->m_box[3] = material_knot_mahogfloor->m_box[3];
						material_knot_marble->m_box[4] = material_knot_mahogfloor->m_box[4];
						material_knot_marble->m_box[5] = material_knot_mahogfloor->m_box[5];
						material_knot_marble->m_model_name = material_knot_mahogfloor->m_model_name;
						material_knot_marble->m_model_pool = material_knot_mahogfloor->m_model_pool;
					}
				}

				{
					material_knot_floreal_tiles = new wr::Model();

					for (auto& m : material_knot_mahogfloor->m_meshes)
					{
						material_knot_floreal_tiles->m_meshes.push_back(std::make_pair(m.first, floreal_tiles_material));
						material_knot_floreal_tiles->m_box[0] = material_knot_mahogfloor->m_box[0];
						material_knot_floreal_tiles->m_box[1] = material_knot_mahogfloor->m_box[1];
						material_knot_floreal_tiles->m_box[2] = material_knot_mahogfloor->m_box[2];
						material_knot_floreal_tiles->m_box[3] = material_knot_mahogfloor->m_box[3];
						material_knot_floreal_tiles->m_box[4] = material_knot_mahogfloor->m_box[4];
						material_knot_floreal_tiles->m_box[5] = material_knot_mahogfloor->m_box[5];
						material_knot_floreal_tiles->m_model_name = material_knot_mahogfloor->m_model_name;
						material_knot_floreal_tiles->m_model_pool = material_knot_mahogfloor->m_model_pool;
					}
				}

				{
					material_knot_bw_tiles = new wr::Model();

					for (auto& m : material_knot_mahogfloor->m_meshes)
					{
						material_knot_bw_tiles->m_meshes.push_back(std::make_pair(m.first, bw_tiles_material));
						material_knot_bw_tiles->m_box[0] = material_knot_mahogfloor->m_box[0];
						material_knot_bw_tiles->m_box[1] = material_knot_mahogfloor->m_box[1];
						material_knot_bw_tiles->m_box[2] = material_knot_mahogfloor->m_box[2];
						material_knot_bw_tiles->m_box[3] = material_knot_mahogfloor->m_box[3];
						material_knot_bw_tiles->m_box[4] = material_knot_mahogfloor->m_box[4];
						material_knot_bw_tiles->m_box[5] = material_knot_mahogfloor->m_box[5];
						material_knot_bw_tiles->m_model_name = material_knot_mahogfloor->m_model_name;
						material_knot_bw_tiles->m_model_pool = material_knot_mahogfloor->m_model_pool;
					}
				}

			}
		}

		void ReleaseResources()
		{
			model_pool.reset();
			texture_pool.reset();
			material_pool.reset();
		};

	}


	static std::shared_ptr<DebugCamera> camera;
	static std::shared_ptr<wr::LightNode> directional_light_node;
	static std::shared_ptr<wr::MeshNode> test_model;
	static float t = 0;

	void CreateScene(wr::SceneGraph* scene_graph, wr::Window* window)
	{
		camera = scene_graph->CreateChild<DebugCamera>(nullptr, 90.f, (float)window->GetWidth() / (float)window->GetHeight());
		camera->SetPosition({ -100.f, 200.f, -325.f });
		camera->SetRotation({-20._deg, 180._deg, 0._deg});
		camera->SetSpeed(60.0f);
		
		scene_graph->m_skybox = resources::equirectangular_environment_map;
		auto skybox = scene_graph->CreateChild<wr::SkyboxNode>(nullptr, resources::equirectangular_environment_map);


		// Geometry
		auto platform1 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::plane_model);

		platform1->SetPosition({ 0, 0, 0 });
		platform1->SetScale({ 500, 500, 1 });
		platform1->SetRotation({ 90_deg, 0, 0 });

		std::shared_ptr<wr::MeshNode> models[10] = {
			scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_brick_tiles),
			scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_leather),
			scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_gold),
			scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_blue_tiles),
			scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_marble),
			scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_metal),
			scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_red_black_pattern),
			scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_mahogfloor),
			scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_bw_tiles),
			scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_floreal_tiles)
		};

		models[9]->SetPosition({ -360,	0	,	-120 });
		models[8]->SetPosition({-180,	0	,	-120 });
		models[7]->SetPosition({ 0,		0	,	-120 });
		models[6]->SetPosition({ +180,		0	,-120 });
		models[5]->SetPosition({ +360,		0	,	-120 });

		models[4]->SetPosition({ -360,0	,	120 });
		models[3]->SetPosition({ -180,		0	,	120 });
		models[2]->SetPosition({ 0,				0	,120 });
		models[1]->SetPosition({ +180,0	,	120 });
		models[0]->SetPosition({ +360,		0	,	120 });

		models[0]->SetRotation({ 0,0,0 });
		models[1]->SetRotation({ 0,0,0 });
		models[2]->SetRotation({ 0,0,0 });
		models[3]->SetRotation({ 0,0,0 });
		models[4]->SetRotation({ 0,0,0 });
		models[5]->SetRotation({ 0,180_deg,0 });
		models[6]->SetRotation({ 0,180_deg,0 });
		models[7]->SetRotation({ 0,180_deg,0 });
		models[8]->SetRotation({ 0,180_deg,0 });
		models[9]->SetRotation({ 0,180_deg,0 });

		directional_light_node = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::DIRECTIONAL, DirectX::XMVECTOR{ 0, 1, 0 });
		directional_light_node->SetDirectional({ 136._deg, 0, 0 }, { 4, 4, 4});
	}

	void UpdateScene()
	{
		t += 10.f * ImGui::GetIO().DeltaTime;

		camera->Update(ImGui::GetIO().DeltaTime);
	}
} /* cube_scene */

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

		static wr::Model* cube_models[10];
		static wr::Model* material_knobs[10];

		static wr::MaterialHandle rusty_metal_material;

		static wr::MaterialHandle material_handles[10];

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

			wr::TextureHandle red_black_albedo = texture_pool->Load("resources/materials/dragon_egg/albedo.png", true, true);
			wr::TextureHandle red_black_normal = texture_pool->Load("resources/materials/dragon_egg/normal.png", false, true);
			wr::TextureHandle red_black_roughness = texture_pool->Load("resources/materials/dragon_egg/roughness.png", false, true);
			wr::TextureHandle red_black_metallic = texture_pool->Load("resources/materials/dragon_egg/metallic.png", false, true);

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
				material_handles[0] = material_pool->Create();

				wr::Material* mahogfloor_material_internal = material_pool->GetMaterial(material_handles[0].m_id);

				mahogfloor_material_internal->SetAlbedo(mahogfloor_albedo);
				mahogfloor_material_internal->SetNormal(mahogfloor_normal);
				mahogfloor_material_internal->SetRoughness(mahogfloor_roughness);
				mahogfloor_material_internal->SetMetallic(mahogfloor_metallic);

				// Create Material
				material_handles[1] = material_pool->Create();

				wr::Material* red_black_pattern_internal = material_pool->GetMaterial(material_handles[1].m_id);

				red_black_pattern_internal->SetAlbedo(red_black_albedo);
				red_black_pattern_internal->SetNormal(red_black_normal);
				red_black_pattern_internal->SetRoughness(red_black_roughness);
				red_black_pattern_internal->SetMetallic(red_black_metallic);

				// Create Material
				material_handles[2] = material_pool->Create();

				wr::Material* metal_material_internal = material_pool->GetMaterial(material_handles[2].m_id);

				metal_material_internal->SetAlbedo(metal_albedo);
				metal_material_internal->SetNormal(metal_normal);
				metal_material_internal->SetRoughness(metal_roughness);
				metal_material_internal->SetMetallic(metal_metallic);

				material_handles[3] = material_pool->Create();

				wr::Material* brick_tiles_mat_internal = material_pool->GetMaterial(material_handles[3].m_id);

				brick_tiles_mat_internal->SetAlbedo(brick_tiles_albedo);
				brick_tiles_mat_internal->SetNormal(brick_tiles_normal);
				brick_tiles_mat_internal->SetRoughness(brick_tiles_roughness);
				brick_tiles_mat_internal->SetMetallic(brick_tiles_metallic);

				material_handles[4] = material_pool->Create();

				wr::Material* leather_material_internal = material_pool->GetMaterial(material_handles[4].m_id);

				leather_material_internal->SetAlbedo(leather_albedo);
				leather_material_internal->SetNormal(leather_normal);
				leather_material_internal->SetRoughness(leather_roughness);
				leather_material_internal->SetMetallic(leather_metallic);

				material_handles[5] = material_pool->Create();

				wr::Material* blue_tiles_material_internal = material_pool->GetMaterial(material_handles[5].m_id);

				blue_tiles_material_internal->SetAlbedo(blue_tiles_albedo);
				blue_tiles_material_internal->SetNormal(blue_tiles_normal);
				blue_tiles_material_internal->SetRoughness(blue_tiles_roughness);
				blue_tiles_material_internal->SetMetallic(blue_tiles_metallic);

				material_handles[6] = material_pool->Create();

				wr::Material* gold_material_internal = material_pool->GetMaterial(material_handles[6].m_id);

				gold_material_internal->SetAlbedo(gold_albedo);
				gold_material_internal->SetNormal(gold_normal);
				gold_material_internal->SetRoughness(gold_roughness);
				gold_material_internal->SetMetallic(gold_metallic);

				material_handles[7] = material_pool->Create();

				wr::Material* marble_material_internal = material_pool->GetMaterial(material_handles[7].m_id);

				marble_material_internal->SetAlbedo(marble_albedo);
				marble_material_internal->SetNormal(marble_normal);
				marble_material_internal->SetRoughness(marble_roughness);
				marble_material_internal->SetMetallic(marble_metallic);

				material_handles[8] = material_pool->Create();

				wr::Material* floreal_tiles_internal = material_pool->GetMaterial(material_handles[8].m_id);

				floreal_tiles_internal->SetAlbedo(floreal_tiles_albedo);
				floreal_tiles_internal->SetNormal(floreal_tiles_normal);
				floreal_tiles_internal->SetRoughness(floreal_tiles_roughness);
				floreal_tiles_internal->SetMetallic(floreal_tiles_metallic);

				material_handles[9] = material_pool->Create();

				wr::Material* bw_tiles_internal = material_pool->GetMaterial(material_handles[9].m_id);

				bw_tiles_internal->SetAlbedo(bw_tiles_albedo);
				bw_tiles_internal->SetNormal(bw_tiles_normal);
				bw_tiles_internal->SetRoughness(bw_tiles_roughness);
				bw_tiles_internal->SetMetallic(bw_tiles_metallic);
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
					m.second = material_handles[0];
				}
			}

			{
				{
					test_model = model_pool->LoadWithMaterials<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/xbot.fbx");

					cube_model = model_pool->Load<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/cube.fbx");
					for (auto& m : cube_model->m_meshes)
					{
						m.second = material_handles[0];
					}

					//Create the platforms
					for (size_t i = 0; i < 10; ++i)
					{
						cube_models[i] = new wr::Model();

						for (auto& m : cube_model->m_meshes)
						{
							cube_models[i]->m_meshes.push_back(std::make_pair(m.first, material_handles[i]));
							cube_models[i]->m_box[0] = cube_model->m_box[0];
							cube_models[i]->m_box[1] = cube_model->m_box[1];
							cube_models[i]->m_box[2] = cube_model->m_box[2];
							cube_models[i]->m_box[3] = cube_model->m_box[3];
							cube_models[i]->m_box[4] = cube_model->m_box[4];
							cube_models[i]->m_box[5] = cube_model->m_box[5];
							cube_models[i]->m_model_name = cube_model->m_model_name;
							cube_models[i]->m_model_pool = cube_model->m_model_pool;
						}
					}

				}

				{
					material_knobs[0] = model_pool->Load<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/material_ball.fbx");

					for (auto& m : material_knobs[0]->m_meshes)
					{
						m.second = material_handles[0];
					}
				}

				{
					//Create the material models 

					for (size_t i = 1; i < 10; ++i)
					{
						material_knobs[i] = new wr::Model();

						for (auto& m : material_knobs[0]->m_meshes)
						{
							material_knobs[i]->m_meshes.push_back(std::make_pair(m.first, material_handles[i]));
							material_knobs[i]->m_box[0] = material_knobs[0]->m_box[0];
							material_knobs[i]->m_box[1] = material_knobs[0]->m_box[1];
							material_knobs[i]->m_box[2] = material_knobs[0]->m_box[2];
							material_knobs[i]->m_box[3] = material_knobs[0]->m_box[3];
							material_knobs[i]->m_box[4] = material_knobs[0]->m_box[4];
							material_knobs[i]->m_box[5] = material_knobs[0]->m_box[5];
							material_knobs[i]->m_model_name = material_knobs[0]->m_model_name;
							material_knobs[i]->m_model_pool = material_knobs[0]->m_model_pool;
						}
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
	static std::shared_ptr<wr::MeshNode> cube_node;
	static float t = 0;
	static float lerp_t = 0.0f;

	std::shared_ptr<wr::MeshNode> models[10];
	std::shared_ptr<wr::MeshNode> platforms[10];

	static DirectX::XMVECTOR camera_start_pos = { 500.0f, 60.0f, 260.0f };
	static DirectX::XMVECTOR camera_end_pos = { -500.0f, 60.0f, 260.0f };


	void CreateScene(wr::SceneGraph* scene_graph, wr::Window* window)
	{
		camera = scene_graph->CreateChild<DebugCamera>(nullptr, 90.f, (float)window->GetWidth() / (float)window->GetHeight());
		camera->SetPosition(camera_start_pos);
		//camera->SetRotation({ -16._deg, 180._deg, 0._deg });
		camera->SetRotation({ -16._deg, 0._deg, 0._deg });
		camera->SetSpeed(100.0f);

		scene_graph->m_skybox = resources::equirectangular_environment_map;
		auto skybox = scene_graph->CreateChild<wr::SkyboxNode>(nullptr, resources::equirectangular_environment_map);


		// Geometry
		for (size_t i = 0; i < 10; ++i)
		{
			models[i] = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knobs[i]);
			platforms[i] = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::cube_models[i]);
			platforms[i]->SetScale({ 38, 1, 38 });
		}

		models[9]->SetPosition({ -500,	0	,	-160 });
		platforms[9]->SetPosition({ -500, -3, -160 });

		models[8]->SetPosition({ -250,	0	,	-160 });
		platforms[8]->SetPosition({ -250,	-3	,	-160 });

		models[7]->SetPosition({ 0,		0	,	-160 });
		platforms[7]->SetPosition({ 0,		-3	,	-160 });

		models[6]->SetPosition({ +250,		0	,-160 });
		platforms[6]->SetPosition({ +250,		-3	,-160 });

		models[5]->SetPosition({ +500,		0	,	-160 });
		platforms[5]->SetPosition({ +500,		-3	,	-160 });

		models[4]->SetPosition({ -500,0	,	160 });
		platforms[4]->SetPosition({ -500, -3 ,	160 });

		models[3]->SetPosition({ -250,		0	,	160 });
		platforms[3]->SetPosition({ -250,		-3	,	160 });

		models[2]->SetPosition({ 0,				0	,160 });
		platforms[2]->SetPosition({ 0,				-3	,160 });

		models[1]->SetPosition({ +250,0	,	160 });
		platforms[1]->SetPosition({ +250, -3	,	160 });

		models[0]->SetPosition({ +500,		0	,	160 });
		platforms[0]->SetPosition({ +500,		-3	,	160 });

		for (uint32_t i = 0; i < 10; ++i)
		{
			//models[i]->SetScale({ 0.5f, 0.5f, 0.5f });
			models[i]->SetRotation({ 90_deg, 0, 0 });
		}

		directional_light_node = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::DIRECTIONAL, DirectX::XMVECTOR{ 0, 1, 0 });
		directional_light_node->SetDirectional({ 360_deg - 136_deg, 0, 0 }, { 4, 4, 4 });
	}

	void UpdateScene()
	{
		static float waiting = 0.0f;
		static bool start_lerp = false;

		float deltaTime = ImGui::GetIO().DeltaTime;

		//waiting += deltaTime;

		//if (waiting > 20.0f)
		//{
		//	start_lerp = true;
		//}

		//t += 5.f * deltaTime;

		//if (lerp_t < 1.0f && start_lerp)
		//{
		//	lerp_t += deltaTime * 0.05f;

		//	if (lerp_t > 1.0f)
		//	{
		//		lerp_t = 1.0f;
		//	}

		//	DirectX::XMVECTOR new_cam_pos = DirectX::XMVectorLerp(camera_start_pos, camera_end_pos, lerp_t);
		//	camera->SetPosition(new_cam_pos);
		//}

		for (uint32_t i = 0; i < 10; ++i)
		{
			models[i]->SetRotation({ 0, DirectX::XMConvertToRadians(t), 0 });
		}

		camera->Update(deltaTime);
	}
} /* cube_scene */

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
		static wr::Model* material_knob;

		static wr::MaterialHandle rusty_metal_material;

		static wr::MaterialHandle material_handles[10];

		static wr::TextureHandle equirectangular_environment_map;

		static wr::TextureHandle flat_normal;

		void CreateResources(wr::RenderSystem* render_system)
		{
			texture_pool = render_system->CreateTexturePool();
			material_pool = render_system->CreateMaterialPool(256);

			// Load Texture.
			wr::TextureHandle white = texture_pool->LoadFromFile("resources/materials/white.png", false, true);
			wr::TextureHandle black = texture_pool->LoadFromFile("resources/materials/black.png", false, true);
			flat_normal = texture_pool->LoadFromFile("resources/materials/flat_normal.png", false, true);

			wr::TextureHandle metal_splotchy_albedo = texture_pool->LoadFromFile("resources/materials/metal-splotchy-albedo.png", true, true);
			wr::TextureHandle metal_splotchy_normal = texture_pool->LoadFromFile("resources/materials/metal-splotchy-normal-dx.png", false, true);
			wr::TextureHandle metal_splotchy_roughness = texture_pool->LoadFromFile("resources/materials/metal-splotchy-rough.png", false, true);
			wr::TextureHandle metal_splotchy_metallic = texture_pool->LoadFromFile("resources/materials/metal-splotchy-metal.png", false, true);

			wr::TextureHandle mahogfloor_albedo = texture_pool->LoadFromFile("resources/materials/mahogfloor/albedo.png", true, true);
			wr::TextureHandle mahogfloor_normal = texture_pool->LoadFromFile("resources/materials/mahogfloor/normal.png", false, true);
			wr::TextureHandle mahogfloor_roughness = texture_pool->LoadFromFile("resources/materials/mahogfloor/roughness.png", false, true);
			wr::TextureHandle mahogfloor_metallic = texture_pool->LoadFromFile("resources/materials/mahogfloor/metallic.png", false, true);

			wr::TextureHandle red_black_albedo = texture_pool->LoadFromFile("resources/materials/dragon_egg/albedo.png", true, true);
			wr::TextureHandle red_black_normal = texture_pool->LoadFromFile("resources/materials/dragon_egg/normal.png", false, true);
			wr::TextureHandle red_black_roughness = texture_pool->LoadFromFile("resources/materials/dragon_egg/roughness.png", false, true);
			wr::TextureHandle red_black_metallic = texture_pool->LoadFromFile("resources/materials/dragon_egg/metallic.png", false, true);

			wr::TextureHandle metal_albedo = texture_pool->LoadFromFile("resources/materials/greasy_pan/albedo.png", true, true);
			wr::TextureHandle metal_normal = texture_pool->LoadFromFile("resources/materials/greasy_pan/normal.png", false, true);
			wr::TextureHandle metal_roughness = texture_pool->LoadFromFile("resources/materials/greasy_pan/roughness.png", false, true);
			wr::TextureHandle metal_metallic = texture_pool->LoadFromFile("resources/materials/greasy_pan/metallic.png", false, true);

			wr::TextureHandle brick_tiles_albedo = texture_pool->LoadFromFile("resources/materials/brick_tiles/albedo.png", true, true);
			wr::TextureHandle brick_tiles_normal = texture_pool->LoadFromFile("resources/materials/brick_tiles/normal.png", false, true);
			wr::TextureHandle brick_tiles_roughness = texture_pool->LoadFromFile("resources/materials/brick_tiles/roughness.png", false, true);
			wr::TextureHandle brick_tiles_metallic = texture_pool->LoadFromFile("resources/materials/brick_tiles/metallic.png", false, true);

			wr::TextureHandle leather_albedo = texture_pool->LoadFromFile("resources/materials/leather_with_metal/albedo.png", true, true);
			wr::TextureHandle leather_normal = texture_pool->LoadFromFile("resources/materials/leather_with_metal/normal.png", false, true);
			wr::TextureHandle leather_roughness = texture_pool->LoadFromFile("resources/materials/leather_with_metal/roughness.png", false, true);
			wr::TextureHandle leather_metallic = texture_pool->LoadFromFile("resources/materials/leather_with_metal/metallic.png", false, true);

			wr::TextureHandle blue_tiles_albedo = texture_pool->LoadFromFile("resources/materials/blue_tiles/albedo.png", true, true);
			wr::TextureHandle blue_tiles_normal = texture_pool->LoadFromFile("resources/materials/blue_tiles/normal.png", false, true);
			wr::TextureHandle blue_tiles_roughness = texture_pool->LoadFromFile("resources/materials/blue_tiles/roughness.png", false, true);
			wr::TextureHandle blue_tiles_metallic = texture_pool->LoadFromFile("resources/materials/blue_tiles/metallic.png", false, true);

			wr::TextureHandle gold_albedo = texture_pool->LoadFromFile("resources/materials/gold_scuffed/albedo.png", true, true);
			wr::TextureHandle gold_normal = texture_pool->LoadFromFile("resources/materials/gold_scuffed/normal.png", false, true);
			wr::TextureHandle gold_roughness = texture_pool->LoadFromFile("resources/materials/gold_scuffed/roughness.png", false, true);
			wr::TextureHandle gold_metallic = texture_pool->LoadFromFile("resources/materials/gold_scuffed/metallic.png", false, true);

			wr::TextureHandle marble_albedo = texture_pool->LoadFromFile("resources/materials/marble_speckled/albedo.png", true, true);
			wr::TextureHandle marble_normal = texture_pool->LoadFromFile("resources/materials/marble_speckled/normal.png", false, true);
			wr::TextureHandle marble_roughness = texture_pool->LoadFromFile("resources/materials/marble_speckled/roughness.png", false, true);
			wr::TextureHandle marble_metallic = texture_pool->LoadFromFile("resources/materials/marble_speckled/metallic.png", false, true);


			wr::TextureHandle floreal_tiles_albedo = texture_pool->LoadFromFile("resources/materials/floreal_tiles/albedo.png", true, true);
			wr::TextureHandle floreal_tiles_normal = texture_pool->LoadFromFile("resources/materials/floreal_tiles/normal.png", false, true);
			wr::TextureHandle floreal_tiles_roughness = texture_pool->LoadFromFile("resources/materials/floreal_tiles/roughness.png", false, true);
			wr::TextureHandle floreal_tiles_metallic = texture_pool->LoadFromFile("resources/materials/floreal_tiles/metallic.png", false, true);

			wr::TextureHandle bw_tiles_albedo = texture_pool->LoadFromFile("resources/materials/bw_tiles_gold_lining/albedo.png", true, true);
			wr::TextureHandle bw_tiles_normal = texture_pool->LoadFromFile("resources/materials/bw_tiles_gold_lining/normal.png", false, true);
			wr::TextureHandle bw_tiles_roughness = texture_pool->LoadFromFile("resources/materials/bw_tiles_gold_lining/roughness.png", false, true);
			wr::TextureHandle bw_tiles_metallic = texture_pool->LoadFromFile("resources/materials/bw_tiles_gold_lining/metallic.png", false, true);

			equirectangular_environment_map = texture_pool->LoadFromFile("resources/materials/artist_workshop_4k.hdr", false, false);

			{
				// Create Material
				rusty_metal_material = material_pool->Create();

				wr::Material* rusty_metal_internal = material_pool->GetMaterial(rusty_metal_material);

				rusty_metal_internal->SetTexture(wr::MaterialTextureType::ALBEDO, metal_splotchy_albedo);
				rusty_metal_internal->SetTexture(wr::MaterialTextureType::NORMAL, metal_splotchy_normal);
				rusty_metal_internal->SetTexture(wr::MaterialTextureType::ROUGHNESS, metal_splotchy_roughness);
				rusty_metal_internal->SetTexture(wr::MaterialTextureType::METALLIC, metal_splotchy_metallic);

				// Create Material
				material_handles[0] = material_pool->Create();

				wr::Material* mahogfloor_material_internal = material_pool->GetMaterial(material_handles[0]);

				mahogfloor_material_internal->SetTexture(wr::MaterialTextureType::ALBEDO, mahogfloor_albedo);
				mahogfloor_material_internal->SetTexture(wr::MaterialTextureType::NORMAL, mahogfloor_normal);
				mahogfloor_material_internal->SetTexture(wr::MaterialTextureType::ROUGHNESS, mahogfloor_roughness);
				mahogfloor_material_internal->SetTexture(wr::MaterialTextureType::METALLIC, mahogfloor_metallic);

				// Create Material
				material_handles[1] = material_pool->Create();

				wr::Material* red_black_pattern_internal = material_pool->GetMaterial(material_handles[1]);

				red_black_pattern_internal->SetTexture(wr::MaterialTextureType::ALBEDO, red_black_albedo);
				red_black_pattern_internal->SetTexture(wr::MaterialTextureType::NORMAL, red_black_normal);
				red_black_pattern_internal->SetTexture(wr::MaterialTextureType::ROUGHNESS, red_black_roughness);
				red_black_pattern_internal->SetTexture(wr::MaterialTextureType::METALLIC, red_black_metallic);

				// Create Material
				material_handles[2] = material_pool->Create();

				wr::Material* metal_material_internal = material_pool->GetMaterial(material_handles[2]);

				metal_material_internal->SetTexture(wr::MaterialTextureType::ALBEDO, metal_albedo);
				metal_material_internal->SetTexture(wr::MaterialTextureType::NORMAL, metal_normal);
				metal_material_internal->SetTexture(wr::MaterialTextureType::ROUGHNESS, metal_roughness);
				metal_material_internal->SetTexture(wr::MaterialTextureType::METALLIC, metal_metallic);

				material_handles[3] = material_pool->Create();

				wr::Material* brick_tiles_mat_internal = material_pool->GetMaterial(material_handles[3]);

				brick_tiles_mat_internal->SetTexture(wr::MaterialTextureType::ALBEDO, brick_tiles_albedo);
				brick_tiles_mat_internal->SetTexture(wr::MaterialTextureType::NORMAL, brick_tiles_normal);
				brick_tiles_mat_internal->SetTexture(wr::MaterialTextureType::ROUGHNESS, brick_tiles_roughness);
				brick_tiles_mat_internal->SetTexture(wr::MaterialTextureType::METALLIC, brick_tiles_metallic);

				material_handles[4] = material_pool->Create();

				wr::Material* leather_material_internal = material_pool->GetMaterial(material_handles[4]);

				leather_material_internal->SetTexture(wr::MaterialTextureType::ALBEDO, leather_albedo);
				leather_material_internal->SetTexture(wr::MaterialTextureType::NORMAL, leather_normal);
				leather_material_internal->SetTexture(wr::MaterialTextureType::ROUGHNESS, leather_roughness);
				leather_material_internal->SetTexture(wr::MaterialTextureType::METALLIC, leather_metallic);

				material_handles[5] = material_pool->Create();

				wr::Material* blue_tiles_material_internal = material_pool->GetMaterial(material_handles[5]);

				blue_tiles_material_internal->SetTexture(wr::MaterialTextureType::ALBEDO, blue_tiles_albedo);
				blue_tiles_material_internal->SetTexture(wr::MaterialTextureType::NORMAL, blue_tiles_normal);
				blue_tiles_material_internal->SetTexture(wr::MaterialTextureType::ROUGHNESS, blue_tiles_roughness);
				blue_tiles_material_internal->SetTexture(wr::MaterialTextureType::METALLIC, blue_tiles_metallic);

				material_handles[6] = material_pool->Create();

				wr::Material* gold_material_internal = material_pool->GetMaterial(material_handles[6]);

				gold_material_internal->SetTexture(wr::MaterialTextureType::ALBEDO, gold_albedo);
				gold_material_internal->SetTexture(wr::MaterialTextureType::NORMAL, gold_normal);
				gold_material_internal->SetTexture(wr::MaterialTextureType::ROUGHNESS, gold_roughness);
				gold_material_internal->SetTexture(wr::MaterialTextureType::METALLIC, gold_metallic);

				material_handles[7] = material_pool->Create();

				wr::Material* marble_material_internal = material_pool->GetMaterial(material_handles[7]);

				marble_material_internal->SetTexture(wr::MaterialTextureType::ALBEDO, marble_albedo);
				marble_material_internal->SetTexture(wr::MaterialTextureType::NORMAL, marble_normal);
				marble_material_internal->SetTexture(wr::MaterialTextureType::ROUGHNESS, marble_roughness);
				marble_material_internal->SetTexture(wr::MaterialTextureType::METALLIC, marble_metallic);

				material_handles[8] = material_pool->Create();

				wr::Material* floreal_tiles_internal = material_pool->GetMaterial(material_handles[8]);

				floreal_tiles_internal->SetTexture(wr::MaterialTextureType::ALBEDO, floreal_tiles_albedo);
				floreal_tiles_internal->SetTexture(wr::MaterialTextureType::NORMAL, floreal_tiles_normal);
				floreal_tiles_internal->SetTexture(wr::MaterialTextureType::ROUGHNESS, floreal_tiles_roughness);
				floreal_tiles_internal->SetTexture(wr::MaterialTextureType::METALLIC, floreal_tiles_metallic);

				material_handles[9] = material_pool->Create();

				wr::Material* bw_tiles_internal = material_pool->GetMaterial(material_handles[9]);

				bw_tiles_internal->SetTexture(wr::MaterialTextureType::ALBEDO, bw_tiles_albedo);
				bw_tiles_internal->SetTexture(wr::MaterialTextureType::NORMAL, bw_tiles_normal);
				bw_tiles_internal->SetTexture(wr::MaterialTextureType::ROUGHNESS, bw_tiles_roughness);
				bw_tiles_internal->SetTexture(wr::MaterialTextureType::METALLIC, bw_tiles_metallic);
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

				}

				{
					material_knob = model_pool->Load<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/material_ball.fbx");
					for (auto& m : material_knob->m_meshes)
					{
						m.second = material_handles[0];
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
		camera->SetRotation({ -16._deg, 0._deg, 0._deg });
		camera->SetSpeed(100.0f);

		scene_graph->m_skybox = resources::equirectangular_environment_map;
		auto skybox = scene_graph->CreateChild<wr::SkyboxNode>(nullptr, resources::equirectangular_environment_map);

		// Geometry
		for (size_t i = 0; i < 10; ++i)
		{
			std::vector<wr::MaterialHandle> mat_handles(resources::material_knob->m_meshes.size(), resources::material_handles[i]);

			models[i] = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knob);
			models[i]->SetMaterials(mat_handles);
			platforms[i] = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::cube_model);
			platforms[i]->SetMaterials(mat_handles);
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
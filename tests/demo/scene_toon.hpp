
#pragma once

#include "wisp.hpp"
#include "window.hpp"
#include "scene_graph/scene_graph.hpp"
#include "imgui/imgui.hpp"
#include "debug_camera.hpp"

namespace toon_scene
{
	namespace resources
	{
		//////////////////////////
		// RESOURCES
		//////////////////////////

		static std::shared_ptr<wr::ModelPool> model_pool;
		static std::shared_ptr<wr::TexturePool> texture_pool;
		static std::shared_ptr<wr::MaterialPool> material_pool;

		static wr::Model* character_scene;
		static wr::Model* tree_scene;
		static wr::Model* lamp_scene;
		static wr::Model* tombstone_scene;

		static std::vector<wr::MaterialHandle> char_material_handles;
		static std::vector<wr::MaterialHandle> tree_material_handles;
		static std::vector<wr::MaterialHandle> lamp_material_handles;
		static std::vector<wr::MaterialHandle> tombstone_material_handles;

		static wr::TextureHandle equirectangular_environment_map;

		void CreateResources(wr::RenderSystem* render_system)
		{
			texture_pool = render_system->CreateTexturePool();
			material_pool = render_system->CreateMaterialPool(256);

			char_material_handles.resize(27);
			tree_material_handles.resize(85);
			lamp_material_handles.resize(1);
			tombstone_material_handles.resize(1);

			// Load Texture.

			//CHARACTER
			wr::TextureHandle character_anvil_albedo = texture_pool->LoadFromFile("resources/models/toon_scene/textures/09_-_Default_baseColor.jpg", true, true);

			wr::TextureHandle character_char_albedo = texture_pool->LoadFromFile("resources/models/toon_scene/textures/02_-_Default_baseColor.png", true, true);

			wr::TextureHandle character_floor_albedo = texture_pool->LoadFromFile("resources/models/toon_scene/textures/03_-_Default_baseColor.png", true, true);

			wr::TextureHandle character_sword_emissive = texture_pool->LoadFromFile("resources/models/toon_scene/textures/02_-_Default_emissive.jpg", true, true);

			wr::TextureHandle character_ambers_albedo = texture_pool->LoadFromFile("resources/models/toon_scene/textures/08_-_Default_baseColor.png", true, true);
			wr::TextureHandle character_ambers_emissive = texture_pool->LoadFromFile("resources/models/toon_scene/textures/08_-_Default_emissive.jpg", true, true);

			//TREE
			wr::TextureHandle tree_body_albedo = texture_pool->LoadFromFile("resources/models/toon_scene/textures/Mat_A_baseColor.jpeg", true, true);
			wr::TextureHandle tree_water_albedo = texture_pool->LoadFromFile("resources/models/toon_scene/textures/Mat_B_baseColor.png", true, true);
			wr::TextureHandle tree_rocks_shrooms_albedo = texture_pool->LoadFromFile("resources/models/toon_scene/textures/Mat_C_baseColor.jpeg", true, true);
			wr::TextureHandle tree_hill_albedo = texture_pool->LoadFromFile("resources/models/toon_scene/textures/Mat_D_baseColor.jpg", true, true);
			wr::TextureHandle tree_top_albedo = texture_pool->LoadFromFile("resources/models/toon_scene/textures/Mat_E_baseColor.jpeg", true, true);
			wr::TextureHandle tree_foliage_albedo = texture_pool->LoadFromFile("resources/models/toon_scene/textures/Mat_F_baseColor.png", true, true);

			//LAMP
			wr::TextureHandle lamp_albedo = texture_pool->LoadFromFile("resources/models/toon_scene/textures/Lamp_Diffuse-min.png", true, true);
			wr::TextureHandle lamp_emissive = texture_pool->LoadFromFile("resources/models/toon_scene/textures/Lamp_Emissive-min.png", false, true);

			//TOMB
			wr::TextureHandle tomb_albedo = texture_pool->LoadFromFile("resources/models/toon_scene/textures/TX_tomb.jpg", true, true);

			//SKY
			equirectangular_environment_map = texture_pool->LoadFromFile("resources/materials/artist_workshop_4k.hdr", false, false);

			//CHARACTER MATERIALS
			{
				// Character Anvil
				char_material_handles[0] = material_pool->Create(texture_pool.get());

				wr::Material* anvil_material_int = material_pool->GetMaterial(char_material_handles[0]);

				anvil_material_int->SetTexture(wr::TextureType::ALBEDO, character_anvil_albedo);
				anvil_material_int->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.99f);
				anvil_material_int->SetConstant<wr::MaterialConstant::METALLIC>(0.01f);

				// Character Body
				char_material_handles[1] = material_pool->Create(texture_pool.get());

				wr::Material* char_mat_internal = material_pool->GetMaterial(char_material_handles[1]);

				char_mat_internal->SetTexture(wr::TextureType::ALBEDO, character_char_albedo);
				char_mat_internal->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.99f);
				char_mat_internal->SetConstant<wr::MaterialConstant::METALLIC>(0.01f);
				char_mat_internal->SetConstant<wr::MaterialConstant::IS_ALPHA_MASKED>(true);

				// Character Floor
				char_material_handles[2] = material_pool->Create(texture_pool.get());

				wr::Material* floor_mat_int = material_pool->GetMaterial(char_material_handles[2]);

				floor_mat_int->SetTexture(wr::TextureType::ALBEDO, character_floor_albedo);
				floor_mat_int->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.99f);
				floor_mat_int->SetConstant<wr::MaterialConstant::METALLIC>(0.01f);
				floor_mat_int->SetConstant<wr::MaterialConstant::IS_ALPHA_MASKED>(true);

				// Character hammer
				char_material_handles[3] = material_pool->Create(texture_pool.get());

				wr::Material* hammer_mat_int = material_pool->GetMaterial(char_material_handles[3]);

				hammer_mat_int->SetTexture(wr::TextureType::ALBEDO, character_char_albedo);
				hammer_mat_int->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.99f);
				hammer_mat_int->SetConstant<wr::MaterialConstant::METALLIC>(0.01f);

				// Character Sword
				char_material_handles[4] = material_pool->Create(texture_pool.get());

				wr::Material* sword_mat_int = material_pool->GetMaterial(char_material_handles[4]);

				sword_mat_int->SetTexture(wr::TextureType::ALBEDO, character_char_albedo);
				sword_mat_int->SetTexture(wr::TextureType::EMISSIVE, character_sword_emissive);
				sword_mat_int->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.99f);
				sword_mat_int->SetConstant<wr::MaterialConstant::METALLIC>(0.01f);
				sword_mat_int->SetConstant<wr::MaterialConstant::EMISSIVE_MULTIPLIER>(150.0f);

				// Character Shoulderpad 1
				char_material_handles[5] = material_pool->Create(texture_pool.get());

				wr::Material* shoulder_1_int = material_pool->GetMaterial(char_material_handles[5]);

				shoulder_1_int->SetTexture(wr::TextureType::ALBEDO, character_char_albedo);
				shoulder_1_int->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.99f);
				shoulder_1_int->SetConstant<wr::MaterialConstant::METALLIC>(0.01f);

				// Character Shoulderpad 2
				char_material_handles[6] = material_pool->Create(texture_pool.get());

				wr::Material* shoulder_2_int = material_pool->GetMaterial(char_material_handles[6]);

				shoulder_2_int->SetTexture(wr::TextureType::ALBEDO, character_char_albedo);
				shoulder_2_int->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.99f);
				shoulder_2_int->SetConstant<wr::MaterialConstant::METALLIC>(0.01f);

				// Character fire embers
				char_material_handles[7] = material_pool->Create(texture_pool.get());

				wr::Material* embers_int = material_pool->GetMaterial(char_material_handles[7]);

				embers_int->SetTexture(wr::TextureType::ALBEDO, character_ambers_albedo);
				embers_int->SetTexture(wr::TextureType::EMISSIVE, character_ambers_emissive);
				embers_int->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.99f);
				embers_int->SetConstant<wr::MaterialConstant::METALLIC>(0.01f);
				embers_int->SetConstant<wr::MaterialConstant::IS_ALPHA_MASKED>(true);
				embers_int->SetConstant<wr::MaterialConstant::IS_DOUBLE_SIDED>(true);
				embers_int->SetConstant<wr::MaterialConstant::EMISSIVE_MULTIPLIER>(150.0f);

				for (int i = 8; i < char_material_handles.size(); ++i)
				{
					char_material_handles[i] = char_material_handles[7];
				}
			}

			//TREE MATERIALS
			{
				// Tree Body
				wr::MaterialHandle tree_body_mat = material_pool->Create(texture_pool.get());

				wr::Material* tree_body_int = material_pool->GetMaterial(tree_body_mat);

				tree_body_int->SetTexture(wr::TextureType::ALBEDO, tree_body_albedo);
				tree_body_int->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.99f);
				tree_body_int->SetConstant<wr::MaterialConstant::METALLIC>(0.01f);
				tree_body_int->SetConstant<wr::MaterialConstant::IS_ALPHA_MASKED>(false);

				// Tree hill
				wr::MaterialHandle tree_hill_mat = material_pool->Create(texture_pool.get());

				wr::Material* tree_hill_int = material_pool->GetMaterial(tree_hill_mat);

				tree_hill_int->SetTexture(wr::TextureType::ALBEDO, tree_hill_albedo);
				tree_hill_int->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.99f);
				tree_hill_int->SetConstant<wr::MaterialConstant::METALLIC>(0.01f);
				tree_hill_int->SetConstant<wr::MaterialConstant::IS_ALPHA_MASKED>(false);

				// Tree rocks and mushrooms
				wr::MaterialHandle tree_rocks_mat = material_pool->Create(texture_pool.get());

				wr::Material* tree_rocks_int = material_pool->GetMaterial(tree_rocks_mat);

				tree_rocks_int->SetTexture(wr::TextureType::ALBEDO, tree_rocks_shrooms_albedo);
				tree_rocks_int->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.99f);
				tree_rocks_int->SetConstant<wr::MaterialConstant::METALLIC>(0.01f);
				tree_rocks_int->SetConstant<wr::MaterialConstant::IS_ALPHA_MASKED>(false);

				// Tree water
				wr::MaterialHandle tree_water_mat = material_pool->Create(texture_pool.get());

				wr::Material* tree_water_int = material_pool->GetMaterial(tree_water_mat);

				tree_water_int->SetTexture(wr::TextureType::ALBEDO, tree_water_albedo);
				tree_water_int->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.99f);
				tree_water_int->SetConstant<wr::MaterialConstant::METALLIC>(0.01f);
				tree_water_int->SetConstant<wr::MaterialConstant::IS_ALPHA_MASKED>(true);

				// Tree top
				wr::MaterialHandle tree_top_mat = material_pool->Create(texture_pool.get());

				wr::Material* tree_top_int = material_pool->GetMaterial(tree_top_mat);

				tree_top_int->SetTexture(wr::TextureType::ALBEDO, tree_top_albedo);
				tree_top_int->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.99f);
				tree_top_int->SetConstant<wr::MaterialConstant::METALLIC>(0.01f);
				tree_top_int->SetConstant<wr::MaterialConstant::IS_ALPHA_MASKED>(false);

				// Tree foliage
				wr::MaterialHandle tree_foliage_mat = material_pool->Create(texture_pool.get());

				wr::Material* tree_foliage_int = material_pool->GetMaterial(tree_foliage_mat);

				tree_foliage_int->SetTexture(wr::TextureType::ALBEDO, tree_foliage_albedo);
				tree_foliage_int->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.99f);
				tree_foliage_int->SetConstant<wr::MaterialConstant::METALLIC>(0.01f);
				tree_foliage_int->SetConstant<wr::MaterialConstant::IS_ALPHA_MASKED>(true);
				tree_foliage_int->SetConstant<wr::MaterialConstant::IS_DOUBLE_SIDED>(true);


				tree_material_handles[0] = tree_body_mat;
				tree_material_handles[1] = tree_hill_mat;

				for (int i = 2; i < 9; ++i)
				{
					tree_material_handles[i] = tree_rocks_mat;
				}

				tree_material_handles[9] = tree_top_mat;
				tree_material_handles[10] = tree_water_mat;

				for (int i = 11; i < 20; ++i)
				{
					tree_material_handles[i] = tree_rocks_mat;
				}

				for (int i = 20; i < 33; ++i)
				{
					tree_material_handles[i] = tree_foliage_mat;
				}

				tree_material_handles[33] = tree_rocks_mat;
				tree_material_handles[34] = tree_rocks_mat;

				tree_material_handles[35] = tree_foliage_mat;
				tree_material_handles[36] = tree_foliage_mat;

				for (int i = 37; i < 42; ++i)
				{
					tree_material_handles[i] = tree_rocks_mat;
				}

				tree_material_handles[42] = tree_foliage_mat;
				tree_material_handles[43] = tree_foliage_mat;
				tree_material_handles[44] = tree_foliage_mat;

				tree_material_handles[45] = tree_rocks_mat;
				tree_material_handles[46] = tree_rocks_mat;
				tree_material_handles[47] = tree_rocks_mat;
				tree_material_handles[48] = tree_rocks_mat;
				tree_material_handles[49] = tree_rocks_mat;
				tree_material_handles[50] = tree_rocks_mat;
				tree_material_handles[51] = tree_rocks_mat;

				for (int i = 52; i < 82; ++i)
				{
					tree_material_handles[i] = tree_foliage_mat;
				}

				tree_material_handles[82] = tree_rocks_mat;
				tree_material_handles[83] = tree_rocks_mat;

				tree_material_handles[84] = tree_foliage_mat;
			}

			//LAMP MATERIAL
			{
				lamp_material_handles[0] = material_pool->Create(texture_pool.get());

				wr::Material* lamp_mat_int = material_pool->GetMaterial(lamp_material_handles[0]);

				lamp_mat_int->SetTexture(wr::TextureType::ALBEDO, lamp_albedo);
				lamp_mat_int->SetTexture(wr::TextureType::EMISSIVE, lamp_emissive);
				lamp_mat_int->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.99f);
				lamp_mat_int->SetConstant<wr::MaterialConstant::METALLIC>(0.01f);
				lamp_mat_int->SetConstant<wr::MaterialConstant::EMISSIVE_MULTIPLIER>(2.0f);
			}

			//TOMBSTONE MATERIAL
			{
				tombstone_material_handles[0] = material_pool->Create(texture_pool.get());

				wr::Material* tomb_mat_int = material_pool->GetMaterial(tombstone_material_handles[0]);

				tomb_mat_int->SetTexture(wr::TextureType::ALBEDO, tomb_albedo);
				tomb_mat_int->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.99f);
				tomb_mat_int->SetConstant<wr::MaterialConstant::METALLIC>(0.01f);
			}

			model_pool = render_system->CreateModelPool(64_mb, 64_mb);

			{
				{
					character_scene = model_pool->Load<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/toon_scene/sick_blacksmith_ascii.fbx");
				}

				{
					tree_scene = model_pool->Load<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/toon_scene/tree_scene.fbx");
				}

				{
					lamp_scene = model_pool->Load<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/toon_scene/Prop_LampCeiling.fbx");
				}

				{
					tombstone_scene = model_pool->Load<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/toon_scene/tombstone.fbx");
				}

				{
					//auto test = model_pool->Load<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/toon_scene/candle.fbx"); 
				}
			}
		}

		void ReleaseResources()
		{
			model_pool.reset();
			texture_pool.reset();
			material_pool.reset();
		}
	}


	static std::shared_ptr<DebugCamera> camera;
	static std::shared_ptr<wr::LightNode> directional_light_node;
	static float t = 0;

	std::shared_ptr<wr::MeshNode> character;
	std::shared_ptr<wr::MeshNode> tree;
	std::shared_ptr<wr::MeshNode> lamp[3];
	std::shared_ptr<wr::MeshNode> tomb;

	static DirectX::XMVECTOR camera_start_pos = { 500.0f, 60.0f, 260.0f };

	void CreateScene(wr::SceneGraph* scene_graph, wr::Window* window)
	{
		camera = scene_graph->CreateChild<DebugCamera>(nullptr, 90.f, (float)window->GetWidth() / (float)window->GetHeight());
		camera->SetPosition(camera_start_pos);
		camera->SetRotation({ -16._deg, 0._deg, 0._deg });
		camera->SetSpeed(100.0f);

		scene_graph->m_skybox = resources::equirectangular_environment_map;
		auto skybox = scene_graph->CreateChild<wr::SkyboxNode>(nullptr, resources::equirectangular_environment_map);

		// CHARACTER
		character = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::character_scene);
		character->SetMaterials(resources::char_material_handles);

		// TREE
		tree = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::tree_scene);
		tree->SetMaterials(resources::tree_material_handles);

		// LAMPS
		lamp[0] = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::lamp_scene);
		lamp[0]->SetMaterials(resources::lamp_material_handles);

		lamp[0]->SetPosition({ -461.50f, 242.147f, -226.86f });
		lamp[0]->SetRotation({ 0, 52_deg, -12.3_deg });
		lamp[0]->SetScale({ 0.7f, 0.7f, 0.7f });

		lamp[1] = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::lamp_scene);
		lamp[1]->SetMaterials(resources::lamp_material_handles);

		lamp[1]->SetPosition({ -376.06f, 232.807f, -177.99f });
		lamp[1]->SetRotation({ -10.2_deg, -42_deg, 7.9_deg });
		lamp[1]->SetScale({ 0.7f, 0.7f, 0.7f });

		lamp[2] = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::lamp_scene);
		lamp[2]->SetMaterials(resources::lamp_material_handles);

		lamp[2]->SetPosition({ -461.50f, 313.089f, -82.661f });
		lamp[2]->SetRotation({ 0, 100_deg, 0 });
		lamp[2]->SetScale({ 0.7f, 0.7f, 0.7f });

		// TOMB
		tomb = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::tombstone_scene);
		tomb->SetMaterials(resources::tombstone_material_handles);

		tomb->SetPosition({ -446.0f, 57.111f, 13.684f });
		tomb->SetRotation({ -7.8_deg, -136.0_deg, 2.3_deg });
		tomb->SetScale({ 14.0f, 14.0f, 14.0f });


		character->SetPosition({ -500,	0	,	-160 });

		directional_light_node = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::DIRECTIONAL, DirectX::XMVECTOR{ 0, 1, 0 });
		directional_light_node->SetDirectional({ 360_deg - 136_deg, 0, 0 }, { 4, 4, 4 });
	}

	void UpdateScene()
	{
		float deltaTime = ImGui::GetIO().DeltaTime;

		camera->Update(deltaTime);
	}
} /* cube_scene */
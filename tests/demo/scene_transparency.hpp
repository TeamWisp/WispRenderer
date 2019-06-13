#pragma once

#include "wisp.hpp"
#include "window.hpp"
#include "scene_graph/scene_graph.hpp"
#include "imgui/imgui.hpp"
#include "debug_camera.hpp"
#include "spline_node.hpp"

namespace transparency_scene
{
	namespace resources
	{
		//////////////////////////
		// RESOURCES
		//////////////////////////

		static std::shared_ptr<wr::ModelPool> model_pool;
		static std::shared_ptr<wr::TexturePool> texture_pool;
		static std::shared_ptr<wr::MaterialPool> material_pool;

		//static wr::Model* plane_model;
		static wr::Model* test_model;

		static wr::MaterialHandle leaves_material;
		static wr::MaterialHandle checkboard_material;
		static wr::MaterialHandle gray_material;

		static wr::TextureHandle equirectangular_environment_map;

		void CreateResources(wr::RenderSystem* render_system)
		{
			texture_pool = render_system->CreateTexturePool();
			material_pool = render_system->CreateMaterialPool(256);
			model_pool = render_system->CreateModelPool(64_mb, 64_mb);

			wr::TextureHandle leaf_albedo = texture_pool->LoadFromFile("resources/models/plant/tropical_plant.png", true, true);
			wr::TextureHandle checkboard = texture_pool->LoadFromFile("resources/materials/presenceMask_File.png", true, true);

			equirectangular_environment_map = texture_pool->LoadFromFile("resources/materials/Circus_Backstage_3k.hdr", false, false);

			// Create Materials
			leaves_material = material_pool->Create(texture_pool.get());
			wr::Material* leaves_internal = material_pool->GetMaterial(leaves_material);

			leaves_internal->SetTexture(wr::TextureType::ALBEDO, leaf_albedo);
			leaves_internal->SetConstant<wr::MaterialConstant::IS_ALPHA_MASKED>(true);
			leaves_internal->SetConstant<wr::MaterialConstant::IS_DOUBLE_SIDED>(true);
			leaves_internal->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.99f);
			leaves_internal->SetConstant<wr::MaterialConstant::METALLIC>(0.01f);


			checkboard_material = material_pool->Create(texture_pool.get());
			wr::Material* checkboard_internal = material_pool->GetMaterial(checkboard_material);

			checkboard_internal->SetTexture(wr::TextureType::ALBEDO, checkboard);
			checkboard_internal->SetConstant<wr::MaterialConstant::IS_ALPHA_MASKED>(true);
			checkboard_internal->SetConstant<wr::MaterialConstant::IS_DOUBLE_SIDED>(true);
			checkboard_internal->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.99f);
			checkboard_internal->SetConstant<wr::MaterialConstant::METALLIC>(0.01f);

			//Gray mat
			gray_material = material_pool->Create(texture_pool.get());
			wr::Material* gray_internal = material_pool->GetMaterial(gray_material);

			gray_internal->SetConstant<wr::MaterialConstant::COLOR>({ 0.5f, 0.5f, 0.5f });
			gray_internal->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.99f);
			gray_internal->SetConstant<wr::MaterialConstant::METALLIC>(0.01f);

			//plane_model = model_pool->Load<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/plane.fbx");
			test_model = model_pool->Load<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/plant/tropical_2.fbx");
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
	static std::vector<std::shared_ptr<wr::MeshNode>> leaves;
	static std::vector<DirectX::XMVECTOR> leaves_rots;
	static std::vector<DirectX::XMVECTOR> leaves_rots_offsets;
	static std::vector<wr::MaterialHandle> mats;
	static float t = 0;

	void CreateScene(wr::SceneGraph* scene_graph, wr::Window* window)
	{
		camera = scene_graph->CreateChild<DebugCamera>(nullptr, 90.f, (float)window->GetWidth() / (float)window->GetHeight());
		camera->SetPosition({ 0, 0, 2 });
		camera->SetSpeed(10);

		auto skybox = scene_graph->CreateChild<wr::SkyboxNode>(nullptr, resources::equirectangular_environment_map);
		auto scene = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::test_model);

		scene->SetScale({ 0.1f, 0.1f, 0.1f });

		mats.resize(3);
		mats[0] = resources::gray_material;
		mats[1] = resources::leaves_material;
		mats[2] = resources::leaves_material;

		scene->SetMaterials(mats);

		/*auto floor = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::plane_model);
		floor->SetMaterials({ resources::floor_material });
		floor->SetScale({ 50.0f, 50.0f, 1.0f });
		floor->SetRotation({90.0_deg, 0.0f, 0.0f});*/

		// Geometry
		/*for (size_t i = 0; i < 200; ++i)
		{
			auto leaf = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::plane_model);
			leaf->SetMaterials({ resources::leaves_material });
			
			float randomX = (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 50.0f))) - 25.0f;
			float randomY = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 50.0f));
			float randomZ = (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 50.0f))) - 25.0f;

			float xRandRot = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 90.0f));
			float yRandRot = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 90.0f));

			leaf->SetPosition({randomX, randomY, randomZ});
			
			leaves_rots.push_back({xRandRot, yRandRot, 0.0f});
			leaves.push_back(leaf);
		}*/

		//leaves_rots_offsets.resize(200);

		// Lights
		auto point_light_0 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::DIRECTIONAL, DirectX::XMVECTOR{ 2000, 2000, 2000 });
		point_light_0->SetRotation({ 200.0_deg, 0.0f, 0.0f });
	}

	void UpdateScene(wr::SceneGraph* sg)
	{
		t += ImGui::GetIO().DeltaTime;

		//for (size_t i = 0; i < 200; ++i)
		//{
		//	auto pos = leaves[i]->m_position;

		//	if (DirectX::XMVectorGetY(pos) < -4.0f)
		//	{
		//		float randomX = (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 50.0f))) - 25.0f;
		//		float randomZ = (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 50.0f))) - 25.0f;

		//		leaves[i]->SetPosition({ randomX , 50.0f, randomZ });
		//	}
		//	else
		//	{
		//		leaves[i]->SetPosition(DirectX::XMVectorSubtract(pos, DirectX::XMVectorSet(0.0f, 0.001f * t, 0.0f, 0.0f)));
		//	}

		//	leaves_rots_offsets[i] = DirectX::XMVectorAdd(leaves_rots_offsets[i], { 0.01f, 0.01f, 0.0f });

		//	leaves[i]->SetRotation(DirectX::XMVectorAdd(leaves_rots[i], leaves_rots_offsets[i]));
		//}

		//auto pos = test_model->m_position;
		//pos.m128_f32[0] = sin(t * 0.1) * 0.5;
		//test_model->SetPosition(pos);

		camera->Update(ImGui::GetIO().DeltaTime);
	}
} /* cube_scene */

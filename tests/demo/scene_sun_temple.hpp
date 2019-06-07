
#pragma once

#include "wisp.hpp"
#include "window.hpp"
#include "scene_graph/scene_graph.hpp"
#include "scene_graph/light_node.hpp"
#include "imgui/imgui.hpp"
#include "debug_camera.hpp"

namespace sun_temple_scene
{
	namespace resources
	{
		//////////////////////////
		// RESOURCES
		//////////////////////////

		static std::shared_ptr<wr::ModelPool> model_pool;
		static std::shared_ptr<wr::TexturePool> texture_pool;
		static std::shared_ptr<wr::MaterialPool> material_pool;

		static wr::Model* sun_temple_model;
		static wr::Model* test_model;

		static wr::TextureHandle equirectangular_environment_map;

		void CreateResources(wr::RenderSystem* render_system)
		{
			texture_pool = render_system->CreateTexturePool();
			material_pool = render_system->CreateMaterialPool(256);

			// Load Texture.

			{
				equirectangular_environment_map = texture_pool->LoadFromFile("resources/materials/sun_temple/SunTemple_Skybox.hdr", false, false);
			}

			model_pool = render_system->CreateModelPool(128_mb, 64_mb);

			{
				sun_temple_model = model_pool->LoadWithMaterials<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/SunTemple.fbx", true);
				test_model = model_pool->LoadWithMaterials<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/xbot.fbx");
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
	static std::shared_ptr<wr::LightNode> torch_node_1;
	static std::shared_ptr<wr::LightNode> torch_node_2;
	static std::shared_ptr<wr::LightNode> torch_node_3;
	static std::shared_ptr<wr::LightNode> torch_node_4;
	static std::shared_ptr<wr::LightNode> torch_node_5;
	static std::shared_ptr<wr::LightNode> torch_node_6;
	static std::shared_ptr<wr::LightNode> torch_node_7;
	static std::shared_ptr<wr::LightNode> torch_node_8;
	static std::shared_ptr<wr::LightNode> torch_node_9;
	static std::shared_ptr<wr::LightNode> torch_node_10;
	static std::shared_ptr<wr::LightNode> torch_node_11;
	static std::shared_ptr<wr::LightNode> torch_node_12;
	static std::shared_ptr<wr::LightNode> torch_node_13;
	static std::shared_ptr<wr::LightNode> torch_node_14;
	static std::shared_ptr<wr::LightNode> torch_node_15;
	static std::shared_ptr<wr::LightNode> torch_node_16;
	static std::shared_ptr<wr::LightNode> torch_node_17;
	static std::shared_ptr<wr::LightNode> torch_node_18;
	static std::shared_ptr<wr::LightNode> torch_node_19;
	static std::shared_ptr<wr::LightNode> fire_bowl_node_1;
	static std::shared_ptr<wr::LightNode> fire_bowl_node_2;
	static std::shared_ptr<wr::MeshNode> test_model;
	static float t = 0;
	static float lerp_t = 0.0f;

	static DirectX::XMVECTOR camera_start_pos = { 500.0f, 60.0f, 260.0f };
	static DirectX::XMVECTOR camera_end_pos = { -500.0f, 60.0f, 260.0f };


	void CreateScene(wr::SceneGraph* scene_graph, wr::Window* window)
	{
		camera = scene_graph->CreateChild<DebugCamera>(nullptr, 90.f, (float)window->GetWidth() / (float)window->GetHeight());
		camera->SetPosition({ 600.f, 700.f, 250.f });
		camera->SetRotation({ -30._deg, 120._deg, 0._deg });
		camera->SetSpeed(100.0f);
		camera->m_frustum_near = 0.5;

		auto skybox = scene_graph->CreateChild<wr::SkyboxNode>(nullptr, resources::equirectangular_environment_map);

		test_model = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::sun_temple_model);
		
		directional_light_node = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::DIRECTIONAL, DirectX::XMVECTOR{ 0, 1, 0 });
		directional_light_node->SetDirectional({ 187.5_deg, -190_deg, 0 }, { 438.f / 255.f, 282.f / 255.f, 174.f / 255.f });

		/*torch_node_1 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 452.f / 255.f, 176.f / 255.f, 68.f / 255.f });
		torch_node_1->SetPosition({ 790.0, 397.5, 215.0 });
		torch_node_1->SetRadius(500.f);

		torch_node_2 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 452.f / 255.f, 176.f / 255.f, 68.f / 255.f });
		torch_node_2->SetPosition({ 571.0, 397.5, 581.0 });
		torch_node_2->SetRadius(500.f);

		torch_node_3 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 452.f / 255.f, 176.f / 255.f, 68.f / 255.f });
		torch_node_3->SetPosition({ 208.0, 397.5, 774.0 });
		torch_node_3->SetRadius(500.f);

		torch_node_4 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 452.f / 255.f, 176.f / 255.f, 68.f / 255.f });
		torch_node_4->SetPosition({ -204, 397.5, 792.8 });
		torch_node_4->SetRadius(500.f);

		torch_node_5 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 452.f / 255.f, 176.f / 255.f, 68.f / 255.f });
		torch_node_5->SetPosition({ -570, 397.5, 577.2 });
		torch_node_5->SetRadius(500.f);

		torch_node_6 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 452.f / 255.f, 176.f / 255.f, 68.f / 255.f });
		torch_node_6->SetPosition({ -798.0, 397.5, 205.9 });
		torch_node_6->SetRadius(500.f);

		torch_node_7 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 452.f / 255.f, 176.f / 255.f, 68.f / 255.f });
		torch_node_7->SetPosition({ -308.2, 397.5, -783.7 });
		torch_node_7->SetRadius(500.f);

		torch_node_8 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 452.f / 255.f, 176.f / 255.f, 68.f / 255.f });
		torch_node_8->SetPosition({ 317.5, 397.5, -780.4 });
		torch_node_8->SetRadius(500.f);

		torch_node_9 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 452.f / 255.f, 176.f / 255.f, 68.f / 255.f });
		torch_node_9->SetPosition({ 747.5, 397.5, -1802.8 });
		torch_node_9->SetRadius(500.f);

		torch_node_10 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 452.f / 255.f, 176.f / 255.f, 68.f / 255.f });
		torch_node_10->SetPosition({ 242.3, 197.0, -2852.4 });
		torch_node_10->SetRadius(500.f);

		torch_node_11 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 452.f / 255.f, 176.f / 255.f, 68.f / 255.f });
		torch_node_11->SetPosition({ -239.0, 197.0, -2842.3 });
		torch_node_11->SetRadius(500.f);

		torch_node_12 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 452.f / 255.f, 176.f / 255.f, 68.f / 255.f });
		torch_node_12->SetPosition({ -711.2, 197.0, -4317.4 });
		torch_node_12->SetRadius(500.f);

		torch_node_13 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 452.f / 255.f, 176.f / 255.f, 68.f / 255.f });
		torch_node_13->SetPosition({ 723.5, 197.0, -4324.3 });
		torch_node_13->SetRadius(500.f);

		torch_node_14 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 452.f / 255.f, 176.f / 255.f, 68.f / 255.f });
		torch_node_14->SetPosition({ -205.2, 197.0, -4853.7 });
		torch_node_14->SetRadius(500.f);

		torch_node_15 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 452.f / 255.f, 176.f / 255.f, 68.f / 255.f });
		torch_node_15->SetPosition({ 209.4, 197.0, -4854.0 });
		torch_node_15->SetRadius(500.f);

		torch_node_16 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 452.f / 255.f, 176.f / 255.f, 68.f / 255.f });
		torch_node_16->SetPosition({ 207.0, 197.0, -5089.7 });
		torch_node_16->SetRadius(500.f);

		torch_node_17 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 452.f / 255.f, 176.f / 255.f, 68.f / 255.f });
		torch_node_17->SetPosition({ -207.3, 197.0, -5079.4 });
		torch_node_17->SetRadius(500.f);

		torch_node_18 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 452.f / 255.f, 176.f / 255.f, 68.f / 255.f });
		torch_node_18->SetPosition({ -141.5, 197.0, -7084.6 });
		torch_node_18->SetRadius(500.f);

		torch_node_19 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 452.f / 255.f, 176.f / 255.f, 68.f / 255.f });
		torch_node_19->SetPosition({ 167.0, 197.0, -7083.8 });
		torch_node_19->SetRadius(500.f);

		fire_bowl_node_1 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 452.f / 255.f * 2.0, 176.f / 255.f * 2.0, 68.f / 255.f * 2.0 });
		fire_bowl_node_1->SetPosition({ 8.8, 95.0, -5798.1 });
		fire_bowl_node_1->SetRadius(2000.f);

		fire_bowl_node_2 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 452.f / 255.f * 2.0, 176.f / 255.f * 2.0, 68.f / 255.f * 2.0 });
		fire_bowl_node_2->SetPosition({ 5.372, 118.7, -3158.8 });
		fire_bowl_node_2->SetRadius(2000.f);*/
	}

	void UpdateScene(void* scam)
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

		camera->Update(deltaTime);
	}
} /* cube_scene */
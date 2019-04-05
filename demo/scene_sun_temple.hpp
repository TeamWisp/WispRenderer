
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
				equirectangular_environment_map = texture_pool->Load("resources/materials/sun_temple/SunTemple_Skybox.hdr", false, false);
			}

			model_pool = render_system->CreateModelPool(64_mb, 64_mb);

			{
				sun_temple_model = model_pool->LoadWithMaterials<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/SunTemple.fbx");
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
	static std::shared_ptr<wr::MeshNode> test_model;
	static float t = 0;
	static float lerp_t = 0.0f;

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

		test_model = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::sun_temple_model);
		
		directional_light_node = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::DIRECTIONAL, DirectX::XMVECTOR{ 0, 1, 0 });
		directional_light_node->SetDirectional({ 190_deg, -180_deg, 0 }, { 8, 8, 8 });
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

		camera->Update(deltaTime);
	}
} /* cube_scene */
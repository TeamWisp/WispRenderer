
#pragma once

#include "wisp.hpp"
#include "window.hpp"
#include "scene_graph/scene_graph.hpp"
#include "imgui/imgui.hpp"
#include "debug_camera.hpp"

namespace large_scene
{
	namespace resources
	{
		//////////////////////////
		// RESOURCES
		//////////////////////////

		static std::shared_ptr<wr::ModelPool> model_pool;
		static std::shared_ptr<wr::TexturePool> texture_pool;
		static std::shared_ptr<wr::MaterialPool> material_pool;

		static wr::Model* suntemple_model;
		static wr::Model* room_model;
		static wr::Model* sponza_model;

		static wr::TextureHandle equirectangular_environment_map;

		void CreateResources(wr::RenderSystem* render_system)
		{
			texture_pool = render_system->CreateTexturePool();
			material_pool = render_system->CreateMaterialPool(512);

			equirectangular_environment_map = texture_pool->LoadFromFile("resources/materials/artist_workshop_4k.hdr", false, false);

			model_pool = render_system->CreateModelPool(256_mb, 256_mb);

			{
				suntemple_model = model_pool->LoadWithMaterials<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/SunTemple/SunTemple.fbx");
				room_model = model_pool->LoadWithMaterials<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/room_test.fbx");
				sponza_model = model_pool->LoadWithMaterials<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/sponza.obj");

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

	static DirectX::XMVECTOR camera_start_pos = { 500.0f, 60.0f, 260.0f };


	void CreateScene(wr::SceneGraph* scene_graph, wr::Window* window)
	{
		camera = scene_graph->CreateChild<DebugCamera>(nullptr, 90.f, (float)window->GetWidth() / (float)window->GetHeight());
		camera->SetPosition(camera_start_pos);
		camera->SetRotation({ -16._deg, 0._deg, 0._deg });
		camera->SetSpeed(100.0f);

		scene_graph->m_skybox = resources::equirectangular_environment_map;
		auto skybox = scene_graph->CreateChild<wr::SkyboxNode>(nullptr, resources::equirectangular_environment_map);

		scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::sponza_model);
	}

	void UpdateScene()
	{
		float deltaTime = ImGui::GetIO().DeltaTime;

		camera->Update(deltaTime);
	}
} /* cube_scene */
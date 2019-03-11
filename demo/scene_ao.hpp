
#pragma once

#include "wisp.hpp"
#include "window.hpp"
#include "scene_graph/scene_graph.hpp"
#include "resources.hpp"
#include "imgui/imgui.hpp"
#include "debug_camera.hpp"


namespace ao_scene
{

	static std::shared_ptr<DebugCamera> camera;
	static std::shared_ptr<wr::LightNode> point_light_node;
	static std::shared_ptr<wr::MeshNode> test_model;
	static float t = 0;

	void CreateScene(wr::SceneGraph* scene_graph, wr::Window* window)
	{
		camera = scene_graph->CreateChild<DebugCamera>(nullptr, 90.f, (float)window->GetWidth() / (float)window->GetHeight());
		camera->SetPosition({ 0.f, 7.f, 0.f });
		camera->SetRotation({ -30._deg, 0_deg, 0._deg });
		camera->SetSpeed(60.0f);

		scene_graph->m_skybox = resources::equirectangular_environment_map;
		auto skybox = scene_graph->CreateChild<wr::SkyboxNode>(nullptr, resources::equirectangular_environment_map);

#ifdef DRAGON
		auto dragon = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::dragon_model);
		//dragon->SetScale({ 10.f, 10.f, 10.f });
#endif // DRAGON

#ifdef HAIRBALL
		auto hairball = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::hairball_model);
#endif // HAIRBALL

#ifdef SUN_TEMPLE
		auto suntemple = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::sun_model);
#endif // SUN_TEMPLE

#ifdef CAR
		auto car = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::car_model);
#endif // SUN_TEMPLE
		
		// Geometry
	//	auto platform1 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::plane_model);

		point_light_node = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 0, 1, 0 });
		point_light_node->SetDirectional({ 136._deg, 0, 0 }, { 4, 4, 4 });
	}

	void UpdateScene()
	{
		t += 10.f * ImGui::GetIO().DeltaTime;

		camera->Update(ImGui::GetIO().DeltaTime);
	}
} /* cube_scene */
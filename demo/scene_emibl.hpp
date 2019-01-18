
#pragma once

#include "wisp.hpp"
#include "window.hpp"
#include "scene_graph/scene_graph.hpp"
#include "resources.hpp"
#include "imgui/imgui.hpp"
#include "debug_camera.hpp"

namespace emibl_scene
{

	static std::shared_ptr<DebugCamera> camera;
	static std::shared_ptr<wr::LightNode> directional_light_node;
	static std::shared_ptr<wr::MeshNode> test_model;
	static float t = 0;

	void CreateScene(wr::SceneGraph* scene_graph, wr::Window* window)
	{
		camera = scene_graph->CreateChild<DebugCamera>(nullptr, 90.f, (float)window->GetWidth() / (float)window->GetHeight());
		camera->SetPosition({ 0, -25, -100 });
		camera->SetSpeed(60.0f);
		
		scene_graph->m_skybox = resources::equirectangular_environment_map;
		auto skybox = scene_graph->CreateChild<wr::SkyboxNode>(nullptr, resources::equirectangular_environment_map);


		// Geometry
		auto platform1 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::plane_model);
		auto platform2 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::plane_model);
		auto platform3 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::plane_model);

		platform1->SetPosition({ 0, 0, 0 });
		platform1->SetScale({ 80, 80, 1 });
		platform1->SetRotation({ -90_deg, 0, 0 });

		platform2->SetPosition({ -180, 0, 0 });
		platform2->SetScale({ 80, 80, 1 });
		platform2->SetRotation({ -90_deg, 0, 0 });

		platform3->SetPosition({ +180, 0, 0 });
		platform3->SetScale({ 80, 80, 1 });
		platform3->SetRotation({ -90_deg, 0, 0 });

		auto model1 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_bamboo);
		auto model2 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_plastic);
		auto model3 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_metal);

		model1->SetPosition({ 0, 0, 0 });
		model2->SetPosition({ -180, 0, 0 });
		model3->SetPosition({ +180, 0, 0 });

		model1->SetRotation({ 0,0,180_deg });
		model2->SetRotation({ 0,0,180_deg });
		model3->SetRotation({ 0,0,180_deg });


		directional_light_node = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::DIRECTIONAL, DirectX::XMVECTOR{ 0, 1, 0 });
		directional_light_node->SetDirectional({ 0, 1, 0 }, { 15, 15, 15 });
	}

	void UpdateScene()
	{
		t += 10.f * ImGui::GetIO().DeltaTime;

		camera->Update(ImGui::GetIO().DeltaTime);
	}
} /* cube_scene */
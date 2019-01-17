
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
		camera->SetSpeed(50.0f);

		// Geometry
		auto platform1 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::plane_model);
		/*auto platform2 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::cube_model);
		auto platform3 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::cube_model);*/

		platform1->SetPosition({ 0, 0, 0 });
		platform1->SetScale({ 80, 80, 1 });
		platform1->SetRotation({ -90_deg, 0, 0 });

		//platform2->SetPosition({ 0, 0, 0 });
		//platform2->SetScale({ 10, 1, 10 });

		//platform3->SetPosition({ 22, 0, 0 });
		//platform3->SetScale({ 10, 1, 10 });

		auto model1 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::test_model);
		/*auto model2 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::test_model);
		auto model3 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::test_model);*/

		model1->SetPosition({ 0, 0, 0 });
		/*model2->SetPosition({ 0, 1, 0 });
		model3->SetPosition({ -22, 1, 0 });*/

		model1->SetRotation({ 0,0,180_deg });
		/*model2->SetRotation({ 0,0,180_deg });
		model3->SetRotation({ 0,0,180_deg });*/

		model1->SetPosition({ 0, 0, 0 });
		model1->SetRotation({ 0,0,180_deg });
		model1->SetScale({ 0.1f,0.1f,0.1f });

		//model1->SetScale({ 0.01f,0.01f,0.01f });
		//model2->SetScale({ 0.01f,0.01f,0.01f });
		//model3->SetScale({ 0.01f,0.01f,0.01f });


		directional_light_node = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::DIRECTIONAL, DirectX::XMVECTOR{ 0, 1, 0 });
		directional_light_node->SetDirectional({ 0, -1, 0 }, { 1, 1, 1 });
	}

	void UpdateScene()
	{
		t += 10.f * ImGui::GetIO().DeltaTime;

		camera->Update(ImGui::GetIO().DeltaTime);
	}
} /* cube_scene */
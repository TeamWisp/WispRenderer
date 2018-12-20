#pragma once

#include "wisp.hpp"
#include "window.hpp"
#include "scene_graph/scene_graph.hpp"
#include "resources.hpp"
#include "imgui/imgui.hpp"

namespace pbr_scene
{

	static std::shared_ptr<wr::CameraNode> camera;
	static std::shared_ptr<wr::LightNode> directional_light_node;
	static float t = 0;

	void CreateScene(wr::SceneGraph* scene_graph, wr::Window* window)
	{
		camera = scene_graph->CreateChild<wr::CameraNode>(nullptr, 90.f, (float)window->GetWidth() / (float)window->GetHeight());
		camera->SetPosition({ 0, 0, -1 });

		// Geometry
		auto test_model = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::test_model);
		test_model->SetPosition({ 0,1,0.75 });
		test_model->SetRotation({ 0,0,180_deg });
		//test_model->SetScale({ 1.f,1.f, 1.f });
		test_model->SetScale({ 0.01f,0.01f,0.01f});

		// Lights
		auto point_light_0 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 10, 10, 10 });
		point_light_0->SetRadius(1.8f);
		point_light_0->SetPosition({ 0, 0, 0 });

	}

	void UpdateScene()
	{

		t += 10.f * ImGui::GetIO().DeltaTime;
	}
} /* cube_scene */
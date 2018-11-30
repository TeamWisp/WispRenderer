#pragma once

#include "wisp.hpp"
#include "window.hpp"
#include "scene_graph/scene_graph.hpp"
#include "resources.hpp"
#include "imgui/imgui.hpp"

namespace viknell_scene
{

	static std::shared_ptr<wr::CameraNode> camera;
	static std::shared_ptr<wr::LightNode> directional_light_node;
	static float t = 0;

	void CreateScene(wr::SceneGraph* scene_graph, wr::Window* window)
	{
		camera = scene_graph->CreateChild<wr::CameraNode>(nullptr, 90.f, (float)window->GetWidth() / (float)window->GetHeight());
		camera->SetPosition(0, 0, -3);
		
		// Geometry
		auto floor = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::plane_model);
		auto roof = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::plane_model);
		auto back_wall = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::plane_model);
		auto left_wall = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::plane_model);
		auto right_wall = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::plane_model);
		floor->SetPosition({ 0, 1, 0 });
		floor->SetRotation({ -90, 0, 0 });
		roof->SetPosition({ 0, -1, 0 });
		roof->SetRotation({ 90, 0, 0 });
		back_wall->SetPosition({ 0, 0, 1 });
		left_wall->SetPosition({ -1, 0, 0 });
		left_wall->SetRotation({ 0, -90, 0 });
		right_wall->SetPosition({ 1, 0, 0 });
		right_wall->SetRotation({ 0, 90, 0 });

		// Lights
		auto point_light_0 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 1, 1, 1 });
		point_light_0->SetRadius(15.f);
		point_light_0->SetPosition({ 0, 0, 0 });

		//auto dir_light = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::DIRECTIONAL, DirectX::XMVECTOR{ 1, 1, 1 });
		//dir_light->SetDirection({ 0, 0, 1 });
	}

	void UpdateScene()
	{

		t += 10.f * ImGui::GetIO().DeltaTime;
	}
} /* cube_scene */
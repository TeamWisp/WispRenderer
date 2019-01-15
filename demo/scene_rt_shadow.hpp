
#pragma once

#include "wisp.hpp"
#include "window.hpp"
#include "scene_graph/scene_graph.hpp"
#include "resources.hpp"
#include "imgui/imgui.hpp"
#include "debug_camera.hpp"

namespace rt_shadow_scene
{

	static std::shared_ptr<DebugCamera> camera;
	static std::shared_ptr<wr::LightNode> directional_light_node;
	static float t = 0;

	void CreateScene(wr::SceneGraph* scene_graph, wr::Window* window)
	{
		camera = scene_graph->CreateChild<DebugCamera>(nullptr, 90.f, (float) window->GetWidth() / (float) window->GetHeight());
		camera->SetPosition({-18, -17, -16});
		camera->SetRotation({-30_deg, 30_deg, 0});

		// Geometry
		auto floor = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::pica_scene);
		floor->SetPosition({0, 1, 0});
		floor->SetRotation({0, 0, 180_deg});

		// Lights
		auto dir_light_0 = scene_graph->CreateChild<wr::LightNode>(nullptr, DirectX::XMVECTOR{37_deg, 120.0_deg, 0.0});

		auto dir_light_1 = scene_graph->CreateChild<wr::LightNode>(nullptr, DirectX::XMVECTOR{37_deg, -120.0_deg, 0.0});
	}

	void UpdateScene()
	{
		camera->Update(ImGui::GetIO().DeltaTime);

		t += 10.f * ImGui::GetIO().DeltaTime;
	}
} /* cube_scene */
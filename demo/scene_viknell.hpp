
#pragma once

#include "wisp.hpp"
#include "window.hpp"
#include "scene_graph/scene_graph.hpp"
#include "resources.hpp"
#include "imgui/imgui.hpp"
#include "debug_camera.hpp"

namespace viknell_scene
{

	static std::shared_ptr<DebugCamera> camera;
	static std::shared_ptr<wr::LightNode> directional_light_node;
	static std::shared_ptr<wr::MeshNode> test_model;
	static float t = 0;

	void CreateScene(wr::SceneGraph* scene_graph, wr::Window* window)
	{
		camera = scene_graph->CreateChild<DebugCamera>(nullptr, 90.f, (float)window->GetWidth() / (float)window->GetHeight());
		camera->SetPosition({ 0, 0, -1 });

		scene_graph->m_skybox = resources::equirectangular_environment_map;
		auto skybox = scene_graph->CreateChild<wr::SkyboxNode>(nullptr, resources::equirectangular_environment_map);

		// Geometry
		auto floor = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::plane_model);
		auto roof = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::plane_model);
		auto roof_light = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::light_model);
		auto back_wall = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::plane_model);
		auto left_wall = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::plane_model);
		auto right_wall = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::plane_model);
		test_model = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::test_model);
		auto sphere = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::sphere_model);
		floor->SetPosition({ 0, 1, 0 });
		sphere->SetPosition({ -1, 1, 1 });
		sphere->SetScale({ 0.6f, 0.6f, 0.6f });
		floor->SetRotation({ -90_deg, 0, 0 });
		floor->SetMeshMaterial(0, &resources::rock_material);
		roof->SetPosition({ 0, -1, 0 });
		roof->SetRotation({ 90_deg, 0, 0 });
		roof->SetMeshMaterial(0, &resources::rock_material);
		roof_light->SetPosition({ 0, -0.999, 0 });
		roof_light->SetRotation({ 90_deg, 0, 0 });
		roof_light->SetScale({ 0.7, 0.7, 0.7 });
		roof_light->SetMeshMaterial(0, &resources::light_material);
		back_wall->SetPosition({ 0, 0, 1 });
		back_wall->SetMeshMaterial(0, &resources::rock_material);
		left_wall->SetPosition({ -1, 0, 0 });
		left_wall->SetRotation({ 0, -90_deg, 0 });
		left_wall->SetMeshMaterial(0, &resources::rock_material);
		right_wall->SetPosition({ 1, 0, 0 });
		right_wall->SetRotation({ 0, 90_deg, 0 });
		right_wall->SetMeshMaterial(0, &resources::rock_material);
		test_model->SetPosition({ 0, 1, 0 });
		test_model->SetRotation({ 0,0,180_deg });
		test_model->SetScale({ 0.01f,0.01f,0.01f });
		test_model->SetMeshMaterial(0, &resources::rusty_metal_material);
		test_model->SetMeshMaterial(1, &resources::rusty_metal_material);

		// Lights
		auto point_light_0 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 5, 5, 5 });
		point_light_0->SetRadius(3.0f);
		point_light_0->SetPosition({ 0, 0, -0.3 });

		auto point_light_1 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 5, 0, 0 });
		point_light_1->SetRadius(2.0f);
		point_light_1->SetPosition({ 0.5, 0, -0.3 });

		auto point_light_2 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 0, 0, 5});
		point_light_2->SetRadius(3.0f);
		point_light_2->SetPosition({ -0.7, 0.5, -0.3 });

		//auto dir_light = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::DIRECTIONAL, DirectX::XMVECTOR{ 1, 1, 1 });
	}

	void UpdateScene()
	{
		t += 10.f * ImGui::GetIO().DeltaTime;

		auto pos = test_model->m_position;
		pos.m128_f32[0] = sin(t * 0.1) * 0.5;
		test_model->SetPosition(pos);

		camera->Update(ImGui::GetIO().DeltaTime);
	}
} /* cube_scene */
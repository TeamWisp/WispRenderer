
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
		camera->SetPosition({ -100.f, -200.f, -325.f });
		camera->SetRotation({-20._deg, 15._deg, 0._deg});
		camera->SetSpeed(60.0f);
		
		scene_graph->m_skybox = resources::equirectangular_environment_map;
		auto skybox = scene_graph->CreateChild<wr::SkyboxNode>(nullptr, resources::equirectangular_environment_map);


		// Geometry
		auto platform1 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::plane_model);

		platform1->SetPosition({ 0, 0, 0 });
		platform1->SetScale({ 500, 500, 1 });
		platform1->SetRotation({ -90_deg, 0, 0 });

		auto model0 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_bamboo);
		auto model1 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_bamboo);
		auto model2 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_plastic);
		auto model3 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_metal);
		auto model4 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_bamboo);
		auto model5 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_plastic);
		auto model6 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_metal);
		auto model7 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_bamboo);
		auto model8 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_plastic);

		model0->SetPosition({	-180,	0	,	-180 });
		model1->SetPosition({ 0,		0	,	-180 });
		model2->SetPosition({ +180,		0	,	-180 });
		model3->SetPosition({ -180,		0	,	0 });
		model4->SetPosition({ 0,		0	,	0 });
		model5->SetPosition({ +180,		0	,	0 });
		model6->SetPosition({ -180,		0	,	180 });
		model7->SetPosition({ 0,		0	,	180 });
		model8->SetPosition({ +180,		0	,	180 });

		model0->SetRotation({ 0,0,180_deg });
		model1->SetRotation({ 0,0,180_deg });
		model2->SetRotation({ 0,0,180_deg });
		model3->SetRotation({ 0,0,180_deg });
		model4->SetRotation({ 0,0,180_deg });
		model5->SetRotation({ 0,0,180_deg });
		model6->SetRotation({ 0,0,180_deg });
		model7->SetRotation({ 0,0,180_deg });
		model8->SetRotation({ 0,0,180_deg });

		directional_light_node = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::DIRECTIONAL, DirectX::XMVECTOR{ 0, 1, 0 });
		directional_light_node->SetDirectional({ 0, 1, 0 }, { 15, 15, 15 });
	}

	void UpdateScene()
	{
		t += 10.f * ImGui::GetIO().DeltaTime;

		camera->Update(ImGui::GetIO().DeltaTime);
	}
} /* cube_scene */
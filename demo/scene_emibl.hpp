
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
		camera->SetPosition({ -100.f, 200.f, -325.f });
		camera->SetRotation({-20._deg, 180._deg, 0._deg});
		camera->SetSpeed(60.0f);
		
		scene_graph->m_skybox = resources::equirectangular_environment_map;
		auto skybox = scene_graph->CreateChild<wr::SkyboxNode>(nullptr, resources::equirectangular_environment_map);

		// Geometry
		auto platform1 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::plane_model);

		platform1->SetPosition({ 0, 0, 0 });
		platform1->SetScale({ 500, 500, 1 });
		platform1->SetRotation({ 90_deg, 0, 0 });

		std::shared_ptr<wr::MeshNode> models[10] = {
			scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_titanium),
			scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_copper),
			scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_gold),
			scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_rusted_iron),
			scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_marble),
			scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_metal),
			scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_plastic),
			scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_bamboo),
			scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_scorched_wood),
			scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::material_knot_rubber)
		};

		models[9]->SetPosition({ -360,	0	,	-120 });
		models[8]->SetPosition({-180,	0	,	-120 });
		models[7]->SetPosition({ 0,		0	,	-120 });
		models[6]->SetPosition({ +180,		0	,-120 });
		models[5]->SetPosition({ +360,		0	,	-120 });

		models[4]->SetPosition({ -360,0	,	120 });
		models[3]->SetPosition({ -180,		0	,	120 });
		models[2]->SetPosition({ 0,				0	,120 });
		models[1]->SetPosition({ +180,0	,	120 });
		models[0]->SetPosition({ +360,		0	,	120 });

		models[0]->SetRotation({ 0,0,0 });
		models[1]->SetRotation({ 0,0,0 });
		models[2]->SetRotation({ 0,0,0 });
		models[3]->SetRotation({ 0,0,0 });
		models[4]->SetRotation({ 0,0,0 });
		models[5]->SetRotation({ 0,180_deg,0 });
		models[6]->SetRotation({ 0,180_deg,0 });
		models[7]->SetRotation({ 0,180_deg,0 });
		models[8]->SetRotation({ 0,180_deg,0 });
		models[9]->SetRotation({ 0,180_deg,0 });

		directional_light_node = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::DIRECTIONAL, DirectX::XMVECTOR{ 0, 1, 0 });
		directional_light_node->SetDirectional({ 136._deg, 0, 0 }, { 4, 4, 4});
	}

	void UpdateScene()
	{
		t += 10.f * ImGui::GetIO().DeltaTime;

		camera->Update(ImGui::GetIO().DeltaTime);
	}
} /* cube_scene */
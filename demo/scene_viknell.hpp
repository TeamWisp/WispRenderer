
#pragma once

#include "wisp.hpp"
#include "window.hpp"
#include "scene_graph/scene_graph.hpp"
#include "resources.hpp"
#include "imgui/imgui.hpp"
#include "debug_camera.hpp"
#include "physics_engine.hpp"
#include "phys_node.hpp"

namespace viknell_scene
{

	std::vector<DirectX::XMVECTOR> target_positions =
	{
		{ 0, 0.9, 0},
		{ 2, 0.9, 0},
		{ 1, 0.9, -2.5},
		{ -4, 0.9, 0},
	};
	int current_target_position = 0;

	static std::shared_ptr<DebugCamera> camera;
	static std::shared_ptr<wr::LightNode> directional_light_node;
	static std::shared_ptr<wr::MeshNode> test_model;
	std::shared_ptr<PhysicsMeshNode> target;
	static float t = 0;
		std::vector<std::shared_ptr<PhysicsMeshNode>> bullets;

	void CreateScene(wr::SceneGraph* scene_graph, wr::Window* window, phys::PhysicsEngine& phys_engine)
	{

		for (auto i = 0; i < 20; i++)
		{
			auto bullet = scene_graph->CreateChild<PhysicsMeshNode>(nullptr, resources::sphere_model);
			bullet->SetMass(1.f);
			bullet->SetupSimpleSphereColl(phys_engine, 0.05f);
			bullet->SetPosition({ 0, 200, 0 });
			bullet->SetScale({ 0.05f, 0.05f, 0.05f });
			bullet->SetRestitution(0.7f);
			bullets.push_back(bullet);
		}

		camera = scene_graph->CreateChild<DebugCamera>(nullptr, 90.f, (float)window->GetWidth() / (float)window->GetHeight(), scene_graph, bullets);
		camera->SetupSimpleSphereColl(phys_engine, 0.1f, 0.2f, 1.f);
		camera->SetPosition({ 0, 0, -1 });
		camera->SetSpeed(10);

		scene_graph->m_skybox = resources::equirectangular_environment_map;
		auto skybox = scene_graph->CreateChild<wr::SkyboxNode>(nullptr, resources::equirectangular_environment_map);

		{
			// Lights
			directional_light_node = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::DIRECTIONAL, DirectX::XMVECTOR{ 5, 5, 5 });
			//point_light_0->SetRadius(3.0f);
			directional_light_node->SetRotation({ 20.950, 0.98, 0 });
			directional_light_node->SetPosition({ 0, -6.527, 17.9 });

			auto point_light_1 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 5, 0, 0 });
			point_light_1->SetRadius(2.0f);
			point_light_1->SetPosition({ 0.5, 0, -0.3 });

			auto point_light_2 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 0, 0, 5 });
			point_light_2->SetRadius(3.0f);
			point_light_2->SetPosition({ -0.7, 0.5, -0.3 });

			//auto dir_light = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::DIRECTIONAL, DirectX::XMVECTOR{ 1, 1, 1 });
		}

		auto floor = scene_graph->CreateChild<PhysicsMeshNode>(nullptr, resources::test_model);
		floor->SetupConvex(phys_engine, resources::test_model);
		floor->SetPosition(target_positions[current_target_position]);
		floor->SetRotation({ -180_deg, 0, 0 });
		floor->SetRestitution(1.f);

		target = scene_graph->CreateChild<PhysicsMeshNode>(nullptr, resources::human_model);
		target->SetMass(0.f);
		target->SetupConvex(phys_engine, resources::human_model);
		target->SetPosition({ 0, 0.5, 0 });
		target->SetRotation({ DirectX::XMConvertToRadians(-180.f), 0, 0});
		target->SetScale({ 0.1f, 0.1f, 0.1f });
		target->SetRestitution(0.7f);
	}

	float UpdateScene(phys::PhysicsEngine& phys_engine, wr::SceneGraph& sg)
	{
		float score = 0;

		t += 10.f * ImGui::GetIO().DeltaTime;

		for (auto& b : bullets)
		{
			auto bullet_pos = b->m_rigid_body->getWorldTransform().getOrigin();
			auto target_pos = target->m_rigid_bodys.value()[0]->getWorldTransform().getOrigin();
			auto dist = (bullet_pos - target_pos).length();

			if (dist < 0.5)
			{
				score += 10;
				b->SetPosition({0, 200, 0});

				int new_pos_idx = 0;
				while (new_pos_idx == current_target_position)
				{
					new_pos_idx = 0 + (rand() % static_cast<int>((target_positions.size() - 1) - 0 + 1));
				}
				current_target_position = new_pos_idx;

				target->SetPosition(target_positions[current_target_position]);
			}
		}

		auto pos = directional_light_node->m_position;
		pos.m128_f32[0] = (20.950) + (sin(t * 0.1) * 4);
		pos.m128_f32[1] = (-6.58) + (cos(t * 0.1) * 2);
		directional_light_node->SetPosition(pos);

		camera->Update(phys_engine, sg, ImGui::GetIO().DeltaTime);

		return score;
	}
} /* cube_scene */
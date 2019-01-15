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
	static std::shared_ptr<wr::LightNode> spot_light;
	static unsigned int num_point_lights = 5;
	static std::vector<std::shared_ptr<wr::LightNode>> point_lights;
	static float t = 0;

	void CreateScene(wr::SceneGraph* scene_graph, wr::Window* window)
	{
		camera = scene_graph->CreateChild<DebugCamera>(nullptr, 90.f, (float) window->GetWidth() / (float) window->GetHeight());
		camera->SetPosition({18.75, -14, -8});
		camera->SetRotation({-11_deg, -26.75_deg, 0});

		// Geometry
		auto floor = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::pica_scene);
		floor->SetPosition({0, 1, 0});
		floor->SetRotation({0, 0, 180_deg});

		// Lights
		auto dir_light_0 = scene_graph->CreateChild<wr::LightNode>(nullptr, DirectX::XMVECTOR{-37_deg, -120.0_deg, 0.0});

		spot_light = scene_graph->CreateChild<wr::LightNode>(nullptr, DirectX::XMVECTOR{-11.5, -22.5, -4.25}, 81.25, DirectX::XMVECTOR{-45.0_deg, 0.0, 0.0}, 29_deg, DirectX::XMVECTOR{5.0, 5.0, 0.75});

		float step_size = DirectX::XMConvertToRadians(static_cast<float>(360 / num_point_lights));
		float current_step = 0.0;

		point_lights.reserve(num_point_lights);
		std::shared_ptr<wr::LightNode> point_light;
		for (int i = 0; i < num_point_lights; ++i)
		{
			DirectX::XMVECTOR light_color;
			switch(i % 5) {
				case 0:
					light_color = DirectX::XMVECTOR{2.0, 1.0, 1.0};
					break;
				case 1:
					light_color = DirectX::XMVECTOR{1.0, 2.0, 1.0};
					break;
				case 2:
					light_color = DirectX::XMVECTOR{1.0, 1.0, 2.0};
					break;
				case 3:
					light_color = DirectX::XMVECTOR{2.0, 2.0, 2.0};
					break;
				case 4:
					light_color = DirectX::XMVECTOR{2.0, 1.0, 2.0};
					break;
			}

			point_light = scene_graph->CreateChild<wr::LightNode>(nullptr, DirectX::XMVECTOR{cos(current_step) * 15.0f, -10.0 , sin(current_step) * 15.0f}, 20, light_color);
			point_lights.push_back(point_light);
			
			current_step += step_size;
		}
		
	}

	void UpdateScene()
	{
		camera->Update(ImGui::GetIO().DeltaTime);

		t += 10.f * ImGui::GetIO().DeltaTime;

		for (int i = 0; i < num_point_lights; ++i)
		{
			float x = point_lights[i]->m_position.m128_f32[0];
			float y = point_lights[i]->m_position.m128_f32[1];
			float z = point_lights[i]->m_position.m128_f32[2];

			float s = sin(ImGui::GetIO().DeltaTime);
			float c = cos(ImGui::GetIO().DeltaTime);

			// rotate point
			float xnew = x * c - z * s;
			float znew = x * s + z * c;

			point_lights[i]->SetPosition(DirectX::XMVECTOR{xnew, y, znew});
		}

		DirectX::XMVECTOR light_pos = spot_light->m_position;
		spot_light->SetPosition(DirectX::XMVECTOR{sin(t/10.f) * 20.f, light_pos.m128_f32[1], light_pos.m128_f32[2]});

	}
} /* cube_scene */
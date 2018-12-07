#pragma once

#include "wisp.hpp"
#include "window.hpp"
#include "scene_graph/scene_graph.hpp"
#include "resources.hpp"
#include "imgui/imgui.hpp"

namespace cubes_scene
{

	static std::vector<std::pair<std::shared_ptr<wr::MeshNode>, int>> bg_nodes(500);
	static std::shared_ptr<wr::MeshNode> mesh_node;
	static std::shared_ptr<wr::CameraNode> camera;
	static std::shared_ptr<wr::LightNode> directional_light_node;
	static float t = 0;

	void CreateScene(wr::SceneGraph* scene_graph, wr::Window* window)
	{
		mesh_node = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::cube_model);
		camera = scene_graph->CreateChild<wr::CameraNode>(nullptr, 70.f, (float)window->GetWidth() / (float)window->GetHeight());

		// #### lights
		auto point_light_node = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 1, 0, 0 });
		point_light_node->SetPosition({ 0, 0, -6 });
		point_light_node->SetRadius(5.f);

		directional_light_node = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::DIRECTIONAL, DirectX::XMVECTOR{ 1, 0, 0 });

		auto spot_light_node = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::SPOT, DirectX::XMVECTOR{ 1, 1, 0 });
		spot_light_node->SetPosition({ 0, 1, -6 });
		spot_light_node->SetRadius(5.f);
		spot_light_node->SetAngle(40.f);

		// #### background cubes
		float distance = 20;
		float cube_size = 2.5f;

		float start_x = -30;
		float start_y = -18;
		float max_column_width = 30;

		int column = 0;
		int row = 0;

		srand(time(0));
		int rand_max = 15;

		for (auto& node : bg_nodes)
		{
			node.first = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::cube_model);
			node.first->SetPosition({ start_x + (cube_size * column), start_y + (cube_size * row), distance });
			node.second = (rand() % rand_max + 0);

			column++;
			if (column > max_column_width) { column = 0; row++; }
		}
		// ### background cubes

		camera->SetPosition({ 0, 0, -5 });
	}

	void UpdateScene()
	{
		mesh_node->SetRotation({ sin(t / 2.f) * 20.f, -t * 10, 0 });

		float perc = sin(time(0)) * 0.5f + 0.5f;

		//If only you could write lerp({ 1, 0, 0 }, { 0, 1, 0 }, perc)
		DirectX::XMVECTOR color = DirectX::XMVectorAdd(
			DirectX::XMVectorMultiply({ 1, 0, 0 }, { perc, perc, perc }),
			DirectX::XMVectorMultiply({ 0, 1, 0 }, { 1 - perc, 1 - perc, 1 - perc })
		);

		directional_light_node->SetColor(color);

		for (auto& node : bg_nodes)
		{
			float speed = 16 + node.second;
			switch (node.second)
			{
			case 0:	node.first->SetRotation({ t, 0, sin(t) * speed });	break;
			case 1:	node.first->SetRotation({ t, 0, sin(-t) * speed }); break;
			case 2: node.first->SetRotation({ -t, 0, sin(t) * speed });	break;
			case 3: node.first->SetRotation({ -t, 0, sin(-t) * speed }); break;

			case 4:	node.first->SetRotation({ t, sin(t) * speed, 0 });	break;
			case 5:	node.first->SetRotation({ t, sin(-t) * speed, 0 }); break;
			case 6: node.first->SetRotation({ -t, sin(t) * speed, 0 });	break;
			case 7: node.first->SetRotation({ -t, sin(-t) * speed, 0 }); break;

			case 8:	node.first->SetRotation({ sin(t) * speed, t, 0 });	break;
			case 9:	node.first->SetRotation({ sin(-t) * speed, t, 0 }); break;
			case 10: node.first->SetRotation({ sin(t) * speed, t, 0 });	break;
			case 11: node.first->SetRotation({ sin(-t) * speed, t, 0 }); break;

			case 12: node.first->SetRotation({ sin(t) * speed, 0, t });	break;
			case 13: node.first->SetRotation({ sin(-t) * speed, 0, t }); break;
			case 14: node.first->SetRotation({ sin(t) * speed, 0, -t }); break;
			case 15: node.first->SetRotation({ sin(-t) * speed, 0, -t }); break;
			}
		}

		t += 10.f * ImGui::GetIO().DeltaTime;
	}
} /* cube_scene */
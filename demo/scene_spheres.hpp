
#pragma once

#include "wisp.hpp"
#include "window.hpp"
#include "scene_graph/scene_graph.hpp"
#include "resources.hpp"
#include "imgui/imgui.hpp"
#include "debug_camera.hpp"

namespace spheres_scene
{

	static std::shared_ptr<DebugCamera> camera;
	static std::shared_ptr<wr::LightNode> directional_light_node;
	static std::shared_ptr<wr::MeshNode> test_model;

	static float t = 0;

	float lerp(float v0, float v1, float t) 
	{
		return (1.0f - t) * v0 + t * v1;
	}

	void CreateScene(wr::SceneGraph* scene_graph, wr::Window* window)
	{
		wr::Model* spheres[49];

		for (uint32_t i = 0; i <= 6; ++i)
		{
			for (uint32_t j = 0; j <= 6; ++j)
			{
				wr::Model* model = new wr::Model();
				
				model->m_model_pool = resources::sphere_model->m_model_pool;
				model->m_model_name = resources::sphere_model->m_model_name;
				model->m_box = resources::sphere_model->m_box;

				float roughness = lerp(1.0f, 0.0f, (float)i / 6);
				float metallic = lerp(1.0f, 0.0f, (float)j / 6);

				//Create new material
				wr::MaterialHandle mat = resources::material_pool->Create();
				auto mat_internal = mat.m_pool->GetMaterial(mat.m_id);

				mat_internal->UseAlbedoTexture(false);
				mat_internal->SetUseConstantAlbedo(true);
				mat_internal->SetConstantAlbedo(DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));
				
				mat_internal->SetUseConstantMetallic(true);
				mat_internal->SetConstantMetallic(DirectX::XMFLOAT3(metallic, metallic, metallic));

				mat_internal->SetUseConstantRoughness(true);
				mat_internal->SetConstantRoughness(roughness);

				model->m_meshes.push_back(std::make_pair(resources::sphere_model->m_meshes[0].first, mat));

				spheres[i * 7 + j] = model;
			}
		}


		camera = scene_graph->CreateChild<DebugCamera>(nullptr, 90.f, (float)window->GetWidth() / (float)window->GetHeight());
		camera->SetPosition({0.0f, 0.0f, 5.f });
		camera->SetSpeed(60.0f);
		
		scene_graph->m_skybox = resources::equirectangular_environment_map;
		auto skybox = scene_graph->CreateChild<wr::SkyboxNode>(nullptr, resources::equirectangular_environment_map);


		// Geometry
		std::shared_ptr<wr::MeshNode> models[49];

		for (uint32_t i = 0; i <= 6; ++i)
		{
			for (uint32_t j = 0; j <= 6; ++j)
			{
				models[i * 7 + j] = scene_graph->CreateChild<wr::MeshNode>(nullptr, spheres[i * 7 + j]);

				float x_pos = lerp(10.0f, -10.0f, (float)i / 6);
				float y_pos = lerp(10.0f, -10.0f, (float)j / 6);

				models[i * 7 + j]->SetPosition({ x_pos, y_pos, 0.0f, 0.0f });
			}
		}

		directional_light_node = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::DIRECTIONAL, DirectX::XMVECTOR{ 0, 1, 0 });
		directional_light_node->SetDirectional({ 136._deg, 0, 0 }, { 4, 4, 4});
	}

	void UpdateScene()
	{
		t += 10.f * ImGui::GetIO().DeltaTime;

		camera->Update(ImGui::GetIO().DeltaTime);
	}
} /* spheres_scene */
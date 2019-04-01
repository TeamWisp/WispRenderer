
#pragma once

#include "wisp.hpp"
#include "window.hpp"
#include "scene_graph/scene_graph.hpp"
#include "imgui/imgui.hpp"
#include "debug_camera.hpp"

namespace spheres_scene
{

	//////////////////////////
	// RESOURCES
	//////////////////////////

	namespace resources
	{
		static std::shared_ptr<wr::ModelPool> model_pool;
		static std::shared_ptr<wr::TexturePool> texture_pool;
		static std::shared_ptr<wr::MaterialPool> material_pool;

		static wr::Model* sphere_model;
		static wr::Model* test_model;

		static wr::TextureHandle equirectangular_environment_map;

		static wr::MaterialHandle spheres_material;

		void CreateResources(wr::RenderSystem* render_system)
		{
			texture_pool = render_system->CreateTexturePool();
			material_pool = render_system->CreateMaterialPool(256);
			model_pool = render_system->CreateModelPool(64_mb, 64_mb);
		
			wr::TextureHandle spheres_albedo = texture_pool->Load("resources/materials/bw_tiles_gold_lining/albedo.png", true, true);
			wr::TextureHandle spheres_normal = texture_pool->Load("resources/materials/bw_tiles_gold_lining/normal.png", false, true);
			wr::TextureHandle spheres_roughness = texture_pool->Load("resources/materials/bw_tiles_gold_lining/roughness.png", false, true);
			wr::TextureHandle spheres_metallic = texture_pool->Load("resources/materials/bw_tiles_gold_lining/metallic.png", false, true);

			equirectangular_environment_map = texture_pool->Load("resources/materials/Circus_Backstage_3k.hdr", false, false);

			// Create Materials
			spheres_material = material_pool->Create();

			wr::Material* spheres_material_internal = material_pool->GetMaterial(spheres_material);

			spheres_material_internal->SetAlbedo(spheres_albedo);
			spheres_material_internal->SetNormal(spheres_normal);
			spheres_material_internal->SetRoughness(spheres_roughness);
			spheres_material_internal->SetMetallic(spheres_metallic);

			// Create Models
			test_model = model_pool->LoadWithMaterials<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/xbot.fbx");

			sphere_model = model_pool->Load<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/sphere.fbx");
			for (auto& m : sphere_model->m_meshes)
			{
				m.second = spheres_material;
			}
		}

		void ReleaseResources()
		{
			model_pool.reset();
			texture_pool.reset();
			material_pool.reset();
		};
	}


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

		//for (uint32_t i = 0; i <= 6; ++i)
		//{
		//	for (uint32_t j = 0; j <= 6; ++j)
		//	{
		//		wr::Model* model = new wr::Model();
		//		
		//		model->m_model_pool = resources::sphere_model->m_model_pool;
		//		model->m_model_name = resources::sphere_model->m_model_name;
		//		model->m_box = resources::sphere_model->m_box;

		//		float roughness = lerp(1.0f, 0.0f, (float)i / 6);
		//		float metallic = lerp(1.0f, 0.0f, (float)j / 6);

		//		//Create new material
		//		wr::MaterialHandle mat = resources::material_pool->Create();
		//		auto mat_internal = mat.m_pool->GetMaterial(mat.m_id);

		//		mat_internal->UseAlbedoTexture(false);
		//		mat_internal->SetUseConstantAlbedo(true);
		//		mat_internal->SetConstantAlbedo(DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));
		//		
		//		mat_internal->SetNormal(resources::flat_normal);

		//		mat_internal->SetUseConstantMetallic(true);
		//		mat_internal->SetConstantMetallic(DirectX::XMFLOAT3(metallic, metallic, metallic));

		//		mat_internal->SetUseConstantRoughness(true);
		//		mat_internal->SetConstantRoughness(roughness);

		//		model->m_meshes.push_back(std::make_pair(resources::sphere_model->m_meshes[0].first, mat));
		//		//model->m_meshes.push_back(std::make_pair(resources::sphere_model->m_meshes[0].first, resources::bamboo_material));

		//		spheres[i * 7 + j] = model;
		//	}
		//}


		camera = scene_graph->CreateChild<DebugCamera>(nullptr, 45.0f, (float)window->GetWidth() / (float)window->GetHeight());
		camera->SetPosition({0.0f, 0.0f, 5.f });
		camera->SetSpeed(60.0f);
		
		scene_graph->m_skybox = resources::equirectangular_environment_map;
		auto skybox = scene_graph->CreateChild<wr::SkyboxNode>(nullptr, resources::equirectangular_environment_map);


		// Geometry
		std::shared_ptr<wr::MeshNode> models[49];

		//for (uint32_t i = 0; i <= 6; ++i)
		//{
		//	for (uint32_t j = 0; j <= 6; ++j)
		//	{
		//		models[i * 7 + j] = scene_graph->CreateChild<wr::MeshNode>(nullptr, spheres[i * 7 + j]);

		//		float x_pos = lerp(10.0f, -10.0f, (float)i / 6);
		//		float y_pos = lerp(10.0f, -10.0f, (float)j / 6);

		//		models[i * 7 + j]->SetPosition({ x_pos, y_pos, 0.0f, 0.0f });
		//		models[i * 7 + j]->SetRotation({ 90.0_deg, 0.0_deg, 0.0_deg });
		//	}
		//}

		for (uint32_t i = 0; i <= 6; ++i)
		{
			for (uint32_t j = 0; j <= 6; ++j)
			{
				models[i * 7 + j] = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::sphere_model);

				float x_pos = lerp(10.0f, -10.0f, (float)i / 6);
				float y_pos = lerp(10.0f, -10.0f, (float)j / 6);

				models[i * 7 + j]->SetPosition({ x_pos, y_pos, 0.0f, 0.0f });
				models[i * 7 + j]->SetRotation({ 90.0_deg, 0.0_deg, 0.0_deg });
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
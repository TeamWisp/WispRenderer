
#pragma once

#include "wisp.hpp"
#include "window.hpp"
#include "scene_graph/scene_graph.hpp"
#include "imgui/imgui.hpp"
#include "debug_camera.hpp"
#include "spline_node.hpp"

namespace shadows_scene
{
	namespace resources
	{
		//////////////////////////
		// RESOURCES
		//////////////////////////

		static std::shared_ptr<wr::ModelPool> model_pool;
		static std::shared_ptr<wr::TexturePool> texture_pool;
		static std::shared_ptr<wr::MaterialPool> material_pool;

		static wr::Model* sphere_model;
		static wr::Model* plane_model;
		static wr::Model* cube_model;
		static wr::Model* tree_model;

		static wr::MaterialHandle white_material;
		static wr::MaterialHandle black_material;
		static wr::MaterialHandle green_material;
		static wr::MaterialHandle branch_material;
		static wr::MaterialHandle leaves_material;

		static wr::TextureHandle equirectangular_environment_map;

		void CreateResources(wr::RenderSystem* render_system)
		{
			texture_pool = render_system->CreateTexturePool();
			material_pool = render_system->CreateMaterialPool(256);
			model_pool = render_system->CreateModelPool(64_mb, 64_mb);

			wr::TextureHandle branch_albedo = texture_pool->LoadFromFile("resources/models/cherry-tree/Trooop_vcols.png", true, true);
			wr::TextureHandle branch_normals = texture_pool->LoadFromFile("resources/models/cherry-tree/Trooop_normals.png", false, true);

			wr::TextureHandle leaves_albedo = texture_pool->LoadFromFile("resources/models/cherry-tree/Cherry branch_D.png", true, true);

			equirectangular_environment_map = texture_pool->LoadFromFile("resources/materials/Circus_Backstage_3k.hdr", false, false);

			// Create Materials
			white_material = material_pool->Create(texture_pool.get());
			wr::Material* white_mat_internal = material_pool->GetMaterial(white_material);

			white_mat_internal->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.8f);
			white_mat_internal->SetConstant<wr::MaterialConstant::METALLIC>(0.2f);
			white_mat_internal->SetConstant<wr::MaterialConstant::COLOR>({ 1.0f, 1.0f, 1.0f });

			black_material = material_pool->Create(texture_pool.get());
			wr::Material* black_mat_internal = material_pool->GetMaterial(black_material);

			black_mat_internal->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.8f);
			black_mat_internal->SetConstant<wr::MaterialConstant::METALLIC>(0.2f);
			black_mat_internal->SetConstant<wr::MaterialConstant::COLOR>({ 0.2f, 0.2f, 0.2f });

			green_material = material_pool->Create(texture_pool.get());
			wr::Material* green_mat_internal = material_pool->GetMaterial(green_material);

			green_mat_internal->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.8f);
			green_mat_internal->SetConstant<wr::MaterialConstant::METALLIC>(0.2f);
			green_mat_internal->SetConstant<wr::MaterialConstant::COLOR>({ 0.0f, 1.0f, 0.1f });

			branch_material = material_pool->Create(texture_pool.get());
			wr::Material* branch_mat_internal = material_pool->GetMaterial(branch_material);

			branch_mat_internal->SetTexture(wr::TextureType::ALBEDO, branch_albedo);
			branch_mat_internal->SetTexture(wr::TextureType::NORMAL, branch_normals);
			branch_mat_internal->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.95f);
			branch_mat_internal->SetConstant<wr::MaterialConstant::METALLIC>(0.01f);

			leaves_material = material_pool->Create(texture_pool.get());
			wr::Material* leaves_mat_internal = material_pool->GetMaterial(leaves_material);

			leaves_mat_internal->SetTexture(wr::TextureType::ALBEDO, leaves_albedo);
			leaves_mat_internal->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.95f);
			leaves_mat_internal->SetConstant<wr::MaterialConstant::METALLIC>(0.01f);
			leaves_mat_internal->SetConstant<wr::MaterialConstant::IS_ALPHA_MASKED>(true);
			leaves_mat_internal->SetConstant<wr::MaterialConstant::IS_DOUBLE_SIDED>(true);

			plane_model = model_pool->Load<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/plane.fbx");
			cube_model = model_pool->Load<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/cube.fbx");
			sphere_model = model_pool->Load<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/sphere.fbx");
			tree_model = model_pool->Load<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/cherry-tree/Good Cherry Tree2.fbx");
		}

		void ReleaseResources()
		{
			model_pool.reset();
			texture_pool.reset();
			material_pool.reset();
		};
	}


	static std::shared_ptr<DebugCamera> camera;
	static std::shared_ptr<SplineNode> camera_spline_node;
	static std::shared_ptr<wr::LightNode> directional_light_node;
	static float t = 0;

	void CreateScene(wr::SceneGraph* scene_graph, wr::Window* window)
	{
		camera = scene_graph->CreateChild<DebugCamera>(nullptr, 90.f, (float) window->GetWidth() / (float) window->GetHeight());
		camera->SetPosition({0, 0, 2});
		camera->SetSpeed(10);

		camera_spline_node = scene_graph->CreateChild<SplineNode>(nullptr, "Camera Spline", false);

		scene_graph->m_skybox = resources::equirectangular_environment_map;
		auto skybox = scene_graph->CreateChild<wr::SkyboxNode>(nullptr, resources::equirectangular_environment_map);

		// Geometry
		auto floor = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::plane_model);
		
		auto cube1 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::cube_model);
		auto long_rect1 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::cube_model);
		auto long_rect2 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::cube_model);
		auto rotated_cube = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::cube_model);
		auto tree = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::tree_model);

		auto sphere1 = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::sphere_model);

		floor->SetPosition({0, 0, 0});
		floor->SetRotation({90_deg, 0, 0});
		floor->SetScale({ 18.0f, 18.0f, 18.0f });
		floor->AddMaterial(resources::white_material);

		sphere1->SetPosition({-1, 1, 0});
		sphere1->SetScale({0.6f, 0.6f, 0.6f});
		sphere1->AddMaterial(resources::white_material);

		cube1->SetPosition({+1, 1, 0});
		cube1->SetRotation({0, 0, 0});
		cube1->SetScale({ 0.2f, 0.2f, 0.2f });
		cube1->AddMaterial(resources::white_material);
		
		long_rect1->SetPosition({ -2.5f, 1.5, 0 });
		long_rect1->SetRotation({ 0, 0, 0 });
		long_rect1->SetScale({ 0.05f, 0.4f, 0.05f });
		long_rect1->AddMaterial(resources::white_material);

		long_rect2->SetPosition({ -3.3f, 1.5, 0 });
		long_rect2->SetRotation({ 0, 0, -10.0_deg });
		long_rect2->SetScale({ 0.05f, 0.4f, 0.05f });
		long_rect2->AddMaterial(resources::white_material);

		rotated_cube->SetPosition({ +3.3, 1.8, 0 });
		rotated_cube->SetRotation({ 45_deg, 30_deg, 0 });
		rotated_cube->SetScale({ 0.2f, 0.2f, 0.2f });
		rotated_cube->AddMaterial(resources::white_material);

		tree->SetPosition({ 3, 1, -5 });
		tree->SetRotation({ 6.5_deg, 95.0_deg, 0 });
		tree->SetScale({ 0.01f, 0.01f, 0.01f });

		std::vector<wr::MaterialHandle> mats(88, resources::branch_material);
		std::fill(mats.begin() + 1, mats.end(), resources::leaves_material);
		tree->SetMaterials(mats);

		// Lights
		auto dir_light = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::DIRECTIONAL, DirectX::XMVECTOR{ 1, 1, 1 });
		dir_light->SetRotation({ 220_deg, 25_deg, 0.0f });
	}

	void UpdateScene()
	{
		t += 10.f * ImGui::GetIO().DeltaTime;

		//auto pos = test_model->m_position;
		//pos.m128_f32[0] = sin(t * 0.1) * 0.5;
		//test_model->SetPosition(pos);

		camera->Update(ImGui::GetIO().DeltaTime);
		camera_spline_node->UpdateSplineNode(ImGui::GetIO().DeltaTime, camera);
	}
} /* cube_scene */

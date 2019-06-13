#pragma once

#include "wisp.hpp"
#include "window.hpp"
#include "scene_graph/scene_graph.hpp"
#include "imgui/imgui.hpp"
#include "debug_camera.hpp"
#include "spline_node.hpp"

namespace transparency_scene
{
	namespace resources
	{
		//////////////////////////
		// RESOURCES
		//////////////////////////

		static std::shared_ptr<wr::ModelPool> model_pool;
		static std::shared_ptr<wr::TexturePool> texture_pool;
		static std::shared_ptr<wr::MaterialPool> material_pool;

		//static wr::Model* plane_model;
		static wr::Model* test_model;

		static wr::MaterialHandle leaves_material;
		static wr::MaterialHandle gray_material;

		static wr::TextureHandle equirectangular_environment_map;

		void CreateResources(wr::RenderSystem* render_system)
		{
			texture_pool = render_system->CreateTexturePool();
			material_pool = render_system->CreateMaterialPool(256);
			model_pool = render_system->CreateModelPool(64_mb, 64_mb);

			wr::TextureHandle leaf_albedo = texture_pool->LoadFromFile("resources/models/plant/tropical_plant.png", true, true);

			equirectangular_environment_map = texture_pool->LoadFromFile("resources/materials/Circus_Backstage_3k.hdr", false, false);

			// Create Materials
			leaves_material = material_pool->Create(texture_pool.get());
			wr::Material* leaves_internal = material_pool->GetMaterial(leaves_material);

			leaves_internal->SetTexture(wr::TextureType::ALBEDO, leaf_albedo);
			leaves_internal->SetConstant<wr::MaterialConstant::IS_ALPHA_MASKED>(true);
			leaves_internal->SetConstant<wr::MaterialConstant::IS_DOUBLE_SIDED>(true);
			leaves_internal->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.99f);
			leaves_internal->SetConstant<wr::MaterialConstant::METALLIC>(0.01f);

			//Gray mat
			gray_material = material_pool->Create(texture_pool.get());
			wr::Material* gray_internal = material_pool->GetMaterial(gray_material);

			gray_internal->SetConstant<wr::MaterialConstant::COLOR>({ 0.5f, 0.5f, 0.5f });
			gray_internal->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.99f);
			gray_internal->SetConstant<wr::MaterialConstant::METALLIC>(0.01f);

			test_model = model_pool->Load<wr::Vertex>(material_pool.get(), texture_pool.get(), "resources/models/plant/tropical_2.fbx");
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
	static std::vector<wr::MaterialHandle> mats;
	static float t = 0;

	void CreateScene(wr::SceneGraph* scene_graph, wr::Window* window, phys::PhysicsEngine& phys_engine)
	{
		camera = scene_graph->CreateChild<DebugCamera>(nullptr, 90.f, (float)window->GetWidth() / (float)window->GetHeight());
		camera->SetPosition({ 0, 0, 2 });
		camera->SetSpeed(10);

		auto skybox = scene_graph->CreateChild<wr::SkyboxNode>(nullptr, resources::equirectangular_environment_map);
		auto scene = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::test_model);

		mats.resize(3);
		mats[0] = resources::gray_material;
		mats[1] = resources::leaves_material;
		mats[2] = resources::leaves_material;

		scene->SetMaterials(mats);		
		scene->SetScale({ 0.1f, 0.1f, 0.1f });

		// Lights
		auto point_light_0 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::DIRECTIONAL, DirectX::XMVECTOR{ 3.0f, 3.0f, 3.0f });
		point_light_0->SetRotation({ 220.0_deg, 0.0f, 0.0f });
	}

	void UpdateScene(wr::SceneGraph* sg)
	{
		t += ImGui::GetIO().DeltaTime;

		camera->Update(ImGui::GetIO().DeltaTime);
	}
} /* cube_scene */

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

		static wr::Model* plane_model;
		static wr::Model* test_model;

		static wr::MaterialHandle leaves_material;
		static wr::MaterialHandle floor_material;
		static wr::MaterialHandle bamboo_material;

		static wr::TextureHandle equirectangular_environment_map;

		void CreateResources(wr::RenderSystem* render_system)
		{
			texture_pool = render_system->CreateTexturePool();
			material_pool = render_system->CreateMaterialPool(256);
			model_pool = render_system->CreateModelPool(64_mb, 64_mb);

			wr::TextureHandle leaf_albedo = texture_pool->LoadFromFile("resources/materials/alpha_leaf.png", true, true);

			equirectangular_environment_map = texture_pool->LoadFromFile("resources/materials/Circus_Backstage_3k.hdr", false, false);

			// Create Materials
			leaves_material = material_pool->Create(texture_pool.get());
			wr::Material* leaves_internal = material_pool->GetMaterial(leaves_material);

			leaves_internal->SetTexture(wr::TextureType::ALBEDO, leaf_albedo);
			leaves_internal->SetConstant<wr::MaterialConstant::IS_ALPHA_MASKED>(true);
			leaves_internal->SetConstant<wr::MaterialConstant::IS_DOUBLE_SIDED>(true);
			leaves_internal->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.5f);
			leaves_internal->SetConstant<wr::MaterialConstant::METALLIC>(0.5f);


			floor_material = material_pool->Create(texture_pool.get());
			wr::Material* floor_internal = material_pool->GetMaterial(floor_material);

			floor_internal->SetConstant<wr::MaterialConstant::COLOR>({1.0f, 1.0f, 0.0f});
			floor_internal->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.99f);
			floor_internal->SetConstant<wr::MaterialConstant::METALLIC>(0.01f);
			floor_internal->SetConstant<wr::MaterialConstant::ALBEDO_UV_SCALE>(20.0f);

			plane_model = model_pool->Load<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/plane.fbx");
			test_model = model_pool->LoadWithMaterials<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/xbot.fbx");
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
	static std::vector<std::shared_ptr<wr::MeshNode>> leaves;
	static std::vector<DirectX::XMVECTOR> leaves_rots;
	static std::vector<DirectX::XMVECTOR> leaves_rots_offsets;
	static float t = 0;

	void CreateScene(wr::SceneGraph* scene_graph, wr::Window* window)
	{
		camera = scene_graph->CreateChild<DebugCamera>(nullptr, 90.f, (float)window->GetWidth() / (float)window->GetHeight());
		camera->SetPosition({ 0, 0, 2 });
		camera->SetSpeed(10);

		auto skybox = scene_graph->CreateChild<wr::SkyboxNode>(nullptr, resources::equirectangular_environment_map);

		auto floor = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::plane_model);
		floor->SetMaterials({ resources::floor_material });
		floor->SetScale({ 50.0f, 50.0f, 1.0f });
		floor->SetRotation({90.0_deg, 0.0f, 0.0f});

		// Geometry
		for (size_t i = 0; i < 200; ++i)
		{
			auto leaf = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::plane_model);
			leaf->SetMaterials({ resources::leaves_material });
			
			float randomX = (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 50.0f))) - 25.0f;
			float randomY = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 50.0f));
			float randomZ = (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 50.0f))) - 25.0f;

			float xRandRot = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 90.0f));
			float yRandRot = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 90.0f));

			leaf->SetPosition({randomX, randomY, randomZ});
			
			leaves_rots.push_back({xRandRot, yRandRot, 0.0f});
			leaves.push_back(leaf);
		}

		leaves_rots_offsets.resize(200);

		// Lights
		auto point_light_0 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::DIRECTIONAL, DirectX::XMVECTOR{ 1, 1, 1 });
		point_light_0->SetRotation({ -50.0_deg, 0.0f, 0.0f });
	}

	void UpdateScene(wr::SceneGraph* sg)
	{
		t += ImGui::GetIO().DeltaTime;

		for (size_t i = 0; i < 200; ++i)
		{
			auto pos = leaves[i]->m_position;

			if (DirectX::XMVectorGetY(pos) < -4.0f)
			{
				float randomX = (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 50.0f))) - 25.0f;
				float randomZ = (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 50.0f))) - 25.0f;

				leaves[i]->SetPosition({ randomX , 50.0f, randomZ });
			}
			else
			{
				leaves[i]->SetPosition(DirectX::XMVectorSubtract(pos, DirectX::XMVectorSet(0.0f, 0.001f * t, 0.0f, 0.0f)));
			}

			leaves_rots_offsets[i] = DirectX::XMVectorAdd(leaves_rots_offsets[i], { 0.01f, 0.01f, 0.0f });

			leaves[i]->SetRotation(DirectX::XMVectorAdd(leaves_rots[i], leaves_rots_offsets[i]));
		}

		//auto pos = test_model->m_position;
		//pos.m128_f32[0] = sin(t * 0.1) * 0.5;
		//test_model->SetPosition(pos);

		camera->Update(ImGui::GetIO().DeltaTime);
	}
} /* cube_scene */

#pragma once

#include "scene_sponza.hpp"

static constexpr bool spawn_physics_balls = false;
static constexpr int num_materials = 30;
static constexpr int num_balls = 600;

SponzaScene::SponzaScene() :
	Scene(256, 20_mb, 20_mb),
	m_sponza_model(nullptr),
	m_skybox({}),
	m_time(0)
{
	m_lights_path = "resources/sponza_lights.json";
}


inline float RandRange(float min, float max)
{
	return min + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (max - min)));
}

inline float RandRangeI(float min, float max)
{
	return min + static_cast <int> (rand()) / (static_cast <int> (RAND_MAX / (max - min)));
}

SponzaScene::~SponzaScene()
{
	if(m_sphere_model)
	{
		m_model_pool->Destroy(m_sphere_model);
		m_model_pool->Destroy(m_sponza_model);
	}
}

void SponzaScene::LoadResources()
{
	// Models
	m_sphere_model = m_model_pool->Load<wr::Vertex>(m_material_pool.get(), m_texture_pool.get(), "resources/models/sphere.fbx");
	m_sponza_model = m_model_pool->LoadWithMaterials<wr::Vertex>(m_material_pool.get(), m_texture_pool.get(), "resources/models/sponza/sponza.obj", false, &m_sponza_model_data);

	// Textures
	m_skybox = m_texture_pool->LoadFromFile("resources/materials/Barce_Rooftop_C_3k.hdr", false, false);

	// Materials
	for (auto i = 0; i < num_materials; i++)
	{
		auto mat = m_material_pool->Create(m_texture_pool.get());
		wr::Material* mat_internal = m_material_pool->GetMaterial(mat);
		mat_internal->SetConstant<wr::MaterialConstant::ROUGHNESS>(RandRange(0, 0.5));
		mat_internal->SetConstant<wr::MaterialConstant::METALLIC>(RandRange(0.5, 1));
		mat_internal->SetConstant<wr::MaterialConstant::EMISSIVE_MULTIPLIER>(RandRange(0, 1) > 0.8 ? 5 : 0);
		mat_internal->SetConstant<wr::MaterialConstant::COLOR>({ RandRange(0.3, 1), RandRange(0.3, 1), RandRange(0.3, 1) });
		m_mirror_materials.push_back(mat);
	}
}

void SponzaScene::BuildScene(unsigned int width, unsigned int height, void* extra)
{
	auto& phys_engine = *reinterpret_cast<phys::PhysicsEngine*>(extra);

	m_camera = m_scene_graph->CreateChild<DebugCamera>(nullptr, 90.f, (float)width / (float)height);
	m_camera->SetPosition({ 0, 1, 11 });
	m_camera->SetSpeed(10);

	m_camera_spline_node = m_scene_graph->CreateChild<SplineNode>(nullptr, "Camera Spline", false);

	auto skybox = m_scene_graph->CreateChild<wr::SkyboxNode>(nullptr, m_skybox);

	// Geometry
	m_sponza_node = m_scene_graph->CreateChild<PhysicsMeshNode>(nullptr, &phys_engine, m_sponza_model);
	m_sponza_node->SetupConvex(phys_engine, m_sponza_model_data);
	m_sponza_node->SetRestitution(1.f);
	m_sponza_node->SetPosition({ 0, -1, 0 });
	m_sponza_node->SetRotation({ 0, 90_deg, 0 });
	m_sponza_node->SetScale({ 0.01f,0.01f,0.01f });

	if constexpr (spawn_physics_balls)
	{
		// Left Ballfall
		for (auto i = 0; i < num_balls / 2; i++)
		{
			auto ball = m_scene_graph->CreateChild<PhysicsMeshNode>(nullptr, &phys_engine, m_sphere_model);
			ball->SetMass(0.004f);
			ball->SetupSimpleSphereColl(phys_engine, 1.f);
			ball->m_rigid_body->setRestitution(0.5);
			ball->m_rigid_body->setFriction(0.2);
			ball->m_rigid_body->setLinearVelocity({ 2, 0, 0 });
			ball->m_rigid_body->setRollingFriction(0);
			ball->m_rigid_body->setSpinningFriction(0);
			ball->SetPosition({ -2.440, 5.5, RandRange(-7.7, 8.8) });
			ball->SetScale({ 0.2f, 0.2f, 0.2f });
			ball->AddMaterial(m_mirror_materials[RandRangeI(0, num_materials - 1)]);
			ball->m_rigid_body->activate(true);
		}
		// Right Ballfall
		for (auto i = 0; i < num_balls / 2; i++)
		{
			auto ball = m_scene_graph->CreateChild<PhysicsMeshNode>(nullptr, &phys_engine, m_sphere_model);
			ball->SetMass(0.004f);
			ball->SetupSimpleSphereColl(phys_engine, 1.f);
			ball->m_rigid_body->setRestitution(0.5);
			ball->m_rigid_body->setFriction(0.2);
			ball->m_rigid_body->setLinearVelocity({ -2, 0, 0 });
			ball->m_rigid_body->setRollingFriction(0);
			ball->m_rigid_body->setSpinningFriction(0);
			ball->SetPosition({ 2.440, 5.5, RandRange(-7.7, 8.8) });
			ball->SetScale({ 0.2f, 0.2f, 0.2f });
			ball->AddMaterial(m_mirror_materials[RandRangeI(0, num_materials - 1)]);
			ball->m_rigid_body->activate(true);
		}
	}

	// Lights
	LoadLightsFromJSON();
}

void SponzaScene::Update(float delta)
{
	m_camera->Update(delta);
	m_camera_spline_node->UpdateSplineNode(delta, m_camera);
}
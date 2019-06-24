/*!
 * Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "scene_sponza.hpp"

#include <algorithm>

static constexpr bool spawn_physics_balls = false;
static constexpr int num_materials = 60;
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

void SponzaScene::LoadResources()
{
	// Models
	m_sphere_model = m_model_pool->Load<wr::Vertex>(m_material_pool.get(), m_texture_pool.get(), "resources/models/sphere.fbx");
	m_logo_core = m_model_pool->Load<wr::Vertex>(m_material_pool.get(), m_texture_pool.get(), "resources/models/logo_core.fbx");
	m_logo_outside = m_model_pool->Load<wr::Vertex>(m_material_pool.get(), m_texture_pool.get(), "resources/models/logo_outside.fbx");
	m_sponza_model = m_model_pool->LoadWithMaterials<wr::Vertex>(m_material_pool.get(), m_texture_pool.get(), "resources/models/sponza/sponza.obj", false, &m_sponza_model_data);

	// Textures
	m_skybox = m_texture_pool->LoadFromFile("resources/materials/Barce_Rooftop_C_3k.hdr", false, false);

	// Materials
	for (auto i = 0; i < num_materials; i++)
	{
		auto mat = m_material_pool->Create(m_texture_pool.get());
		wr::Material* mat_internal = m_material_pool->GetMaterial(mat);
		mat_internal->SetConstant<wr::MaterialConstant::ROUGHNESS>(RandRange(0, 0.35));
		mat_internal->SetConstant<wr::MaterialConstant::METALLIC>(RandRange(0, 1));
		mat_internal->SetConstant<wr::MaterialConstant::EMISSIVE_MULTIPLIER>(RandRange(0, 1) > 0.8 ? 5 : 0);
		mat_internal->SetConstant<wr::MaterialConstant::COLOR>({ RandRange(0.3, 1), RandRange(0.3, 1), RandRange(0.3, 1) });
		m_mirror_materials.push_back(mat);
	}

	m_wisp_material = m_material_pool->Create(m_texture_pool.get());
	wr::Material* mat_internal = m_material_pool->GetMaterial(m_wisp_material);
	mat_internal->SetConstant<wr::MaterialConstant::ROUGHNESS>(0);
	mat_internal->SetConstant<wr::MaterialConstant::METALLIC>(0.9);
	mat_internal->SetConstant<wr::MaterialConstant::EMISSIVE_MULTIPLIER>(0);
	mat_internal->SetConstant<wr::MaterialConstant::COLOR>({ 101.f /255.f, 1, 183.f / 255.f });
}

void SponzaScene::BuildScene(unsigned int width, unsigned int height, void* extra)
{
	auto& phys_engine = *reinterpret_cast<phys::PhysicsEngine*>(extra);

	m_camera = m_scene_graph->CreateChild<DebugCamera>(nullptr, 90.f, (float)width / (float)height);
	//m_camera->SetFovFromFocalLength(m_camera->m_aspect_ratio, m_camera->m_film_size);
	m_camera->SetPosition({ 0, 1, 11 });
	m_camera->m_film_size = 60;
	m_camera->SetSpeed(10);

	m_camera_spline_node = m_scene_graph->CreateChild<SplineNode>(nullptr, "Camera Spline", false, true);
	m_camera_spline_node->LoadSplineFromFile("resources/splines/sponza_lion_camera.spl");
	m_camera_spline_node->m_animate = true;
	m_camera_spline_node->UpdateNaturalSpline();
	m_camera_spline_node->m_speed = 0.2;

	auto skybox = m_scene_graph->CreateChild<wr::SkyboxNode>(nullptr, m_skybox);

	// Geometry
	m_core = m_scene_graph->CreateChild<wr::MeshNode>(nullptr, m_logo_core);
	m_core->SetMaterials({ m_wisp_material });
	m_core->SetScale({ 0.1, 0.1, 0.1});
	m_outside = m_scene_graph->CreateChild<wr::MeshNode>(nullptr, m_logo_outside);
	m_outside->SetMaterials({ m_wisp_material });
	m_outside->SetScale({ 0.1, 0.1, 0.1 });

	m_sponza_node = m_scene_graph->CreateChild<PhysicsMeshNode>(nullptr, m_sponza_model);
	m_sponza_node->SetupTriangleMesh(phys_engine, m_sponza_model_data);
	m_sponza_node->SetRestitution(1.f);
	m_sponza_node->SetPosition({ 0, -1, 0 });
	m_sponza_node->SetRotation({ 0, 90_deg, 0 });
	m_sponza_node->SetScale({ 0.01f,0.01f,0.01f });

	if constexpr (spawn_physics_balls)
	{
		// Left Ballfall
		for (auto i = 0; i < num_balls / 2; i++)
		{
			auto ball = m_scene_graph->CreateChild<PhysicsMeshNode>(nullptr, m_sphere_model);
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
			auto ball = m_scene_graph->CreateChild<PhysicsMeshNode>(nullptr, m_sphere_model);
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
	//LoadLightsFromJSON();

	red_light = m_scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT);
	red_light->SetColor({ 1, 0, 0 });
	red_light->SetPosition({ -3.5, 2, -11 });
	red_light->SetLightSize(5);
	red_light->SetRadius(0);

	green_light = m_scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT);
	green_light->SetColor({ 0, 0, 1 });
	green_light->SetPosition({ 4, 2, -11 });
	green_light->SetLightSize(5);
	green_light->SetRadius(0);

	blue_light = m_scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT);
	blue_light->SetColor({ 0, 0.72, 1 });
	blue_light->SetPosition({ 0.345, 1, -12 });
	blue_light->SetLightSize(5);
	blue_light->SetRadius(0);

	white_light = m_scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT);
	white_light->SetColor({ 1, 1, 1 });
	white_light->SetPosition({ 0, 5, 0 });
	white_light->SetLightSize(5);
	white_light->SetRadius(0);
}

void SponzaScene::Update(float delta)
{
	m_camera->Update(delta);
	m_camera_spline_node->UpdateSplineNode(delta, m_camera);

	m_time += delta * 1.5;
	float blue_delay = 1;
	float delay_rg = 4;
	float logo_delay = 6;
	green_light->SetRadius(std::clamp(m_time - delay_rg, 0.f, 6.f));
	blue_light->SetRadius(std::clamp(m_time - blue_delay, 0.f, 5.f));
	red_light->SetRadius(std::clamp(m_time - delay_rg, 0.f, 6.f));
	white_light->SetRadius(std::clamp(m_time, 0.f, 100.f));

	float logo_alpha = std::clamp((m_time - logo_delay) / 10.f, 0.f, 1.f);
	auto logo_pos = DirectX::XMVectorLerp(start_logo_pos, end_logo_pos, logo_alpha);
	m_core->SetPosition(logo_pos);
	m_outside->SetPosition(logo_pos);
	m_core->SetRotation({ 0, m_time, 0 });

	//m_time = std::fmodf(m_time, 10.f);
}
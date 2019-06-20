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

#include "scene_viknell.hpp"

ViknellScene::ViknellScene() :
	Scene(256, 2_mb, 2_mb),
	m_sphere_model(nullptr),
	m_plane_model(nullptr),
	m_xbot_model(nullptr),
	m_bamboo_material(),
	m_skybox({}),
	m_plane_model_data(nullptr),
	m_time(0)
{
	m_lights_path = "resources/viknell_lights.json";
}

void ViknellScene::LoadResources()
{
	// Models
	m_plane_model = m_model_pool->Load<wr::Vertex>(m_material_pool.get(), m_texture_pool.get(), "resources/models/plane.fbx", &m_plane_model_data);
	m_xbot_model = m_model_pool->LoadWithMaterials<wr::Vertex>(m_material_pool.get(), m_texture_pool.get(), "resources/models/xbot.fbx");
	m_sphere_model = m_model_pool->Load<wr::Vertex>(m_material_pool.get(), m_texture_pool.get(), "resources/models/sphere.fbx");

	// Textures
	wr::TextureHandle bamboo_albedo = m_texture_pool->LoadFromFile("resources/materials/bamboo/bamboo-wood-semigloss-albedo.png", true, true);
	wr::TextureHandle bamboo_normal = m_texture_pool->LoadFromFile("resources/materials/bamboo/bamboo-wood-semigloss-normal.png", false, true);
	wr::TextureHandle bamboo_roughness = m_texture_pool->LoadFromFile("resources/materials/bamboo/bamboo-wood-semigloss-roughness.png", false, true);
	wr::TextureHandle bamboo_metallic = m_texture_pool->LoadFromFile("resources/materials/bamboo/bamboo-wood-semigloss-metal.png", false, true);
	m_skybox = m_texture_pool->LoadFromFile("resources/materials/Barce_Rooftop_C_3k.hdr", false, false);

	// Materials
	m_mirror_material = m_material_pool->Create(m_texture_pool.get());
	wr::Material* mirror_internal = m_material_pool->GetMaterial(m_mirror_material);
	mirror_internal->SetConstant<wr::MaterialConstant::ROUGHNESS>(0);
	mirror_internal->SetConstant<wr::MaterialConstant::METALLIC>(1);
	mirror_internal->SetConstant<wr::MaterialConstant::COLOR>({ 1, 1, 1 });

	m_bamboo_material = m_material_pool->Create(m_texture_pool.get());
	wr::Material* bamboo_material_internal = m_material_pool->GetMaterial(m_bamboo_material);
	bamboo_material_internal->SetTexture(wr::TextureType::ALBEDO, bamboo_albedo);
	bamboo_material_internal->SetTexture(wr::TextureType::NORMAL, bamboo_normal);
	bamboo_material_internal->SetTexture(wr::TextureType::ROUGHNESS, bamboo_roughness);
	bamboo_material_internal->SetTexture(wr::TextureType::METALLIC, bamboo_metallic);
}

void ViknellScene::BuildScene(unsigned int width, unsigned int height, void* extra)
{
	auto& phys_engine = *reinterpret_cast<phys::PhysicsEngine*>(extra);

	m_camera = m_scene_graph->CreateChild<DebugCamera>(nullptr, 90.f, (float)width / (float)height);
	m_camera->SetPosition({ 0, 0, 2 });
	m_camera->SetSpeed(10);

	m_camera_spline_node = m_scene_graph->CreateChild<SplineNode>(nullptr, "Camera Spline", false);

	auto skybox = m_scene_graph->CreateChild<wr::SkyboxNode>(nullptr, m_skybox);

	// Geometry
	auto floor = m_scene_graph->CreateChild<PhysicsMeshNode>(nullptr, m_plane_model);
	auto roof = m_scene_graph->CreateChild<wr::MeshNode>(nullptr, m_plane_model);
	auto back_wall = m_scene_graph->CreateChild<wr::MeshNode>(nullptr, m_plane_model);
	auto left_wall = m_scene_graph->CreateChild<wr::MeshNode>(nullptr, m_plane_model);
	auto right_wall = m_scene_graph->CreateChild<wr::MeshNode>(nullptr, m_plane_model);
	m_xbot_node = m_scene_graph->CreateChild<PhysicsMeshNode>(nullptr, m_xbot_model);
	auto sphere = m_scene_graph->CreateChild<wr::MeshNode>(nullptr, m_sphere_model);

	floor->SetupConvex(phys_engine, m_plane_model_data);
	floor->SetRestitution(1.f);
	floor->SetPosition({ 0, -1, 0 });
	floor->SetRotation({ 90_deg, 0, 0 });
	floor->AddMaterial(m_bamboo_material);
	sphere->SetPosition({ 1, -1, -1 });
	sphere->SetScale({ 0.6f, 0.6f, 0.6f });
	sphere->AddMaterial(m_mirror_material);
	roof->SetPosition({ 0, 1, 0 });
	roof->SetRotation({ -90_deg, 0, 0 });
	roof->AddMaterial(m_bamboo_material);
	back_wall->SetPosition({ 0, 0, -1 });
	back_wall->SetRotation({ 0, 180_deg, 0 });
	back_wall->AddMaterial(m_bamboo_material);
	left_wall->SetPosition({ -1, 0, 0 });
	left_wall->SetRotation({ 0, -90_deg, 0 });
	left_wall->AddMaterial(m_bamboo_material);
	right_wall->SetPosition({ 1, 0, 0 });
	right_wall->SetRotation({ 0, 90_deg, 0 });
	right_wall->AddMaterial(m_bamboo_material);

	m_xbot_node->SetMass(0.01f);
	m_xbot_node->SetupSimpleSphereColl(phys_engine, 0.5);
	m_xbot_node->m_rigid_body->setRestitution(0.4);
	m_xbot_node->m_rigid_body->setFriction(0);
	m_xbot_node->m_rigid_body->setRollingFriction(0);
	m_xbot_node->m_rigid_body->setSpinningFriction(0);
	m_xbot_node->SetPosition({ 0, -1, 0 });
	m_xbot_node->SetRotation({ 0, 180_deg, 0 });
	m_xbot_node->SetScale({ 0.01f,0.01f,0.01f });
	m_xbot_node->m_rigid_body->activate(true);

	// Lights
	/*auto point_light_0 = m_scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::DIRECTIONAL, DirectX::XMVECTOR{ 1, 1, 1 });
	point_light_0->SetRotation({ 20.950f, 0.98f, 0.f });
	point_light_0->SetPosition({ -0.002f, 0.080f, 1.404f });

	auto point_light_1 = m_scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 1, 0, 0 });
	point_light_1->SetRadius(5.0f);
	point_light_1->SetPosition({ 0.5f, 0.f, -0.3f });

	auto point_light_2 = m_scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 0, 0, 1 });
	point_light_2->SetRadius(5.0f);
	point_light_2->SetPosition({ -0.5f, 0.5f, -0.3f });*/

	LoadLightsFromJSON();
}

void ViknellScene::Update(float delta)
{
	m_camera->Update(delta);
	m_camera_spline_node->UpdateSplineNode(ImGui::GetIO().DeltaTime, m_camera);
}
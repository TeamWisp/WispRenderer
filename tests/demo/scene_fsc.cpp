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

#include "scene_fsc.hpp"


FscScene::FscScene() :
	Scene(512, 200_mb, 200_mb),
	m_stalker_model(nullptr),
	m_skybox({}),
	m_time(0)
{
	//m_lights_path = "resources/sponza_lights.json";
}


inline float RandRange(float min, float max)
{
	return min + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (max - min)));
}

inline float RandRangeI(float min, float max)
{
	return min + static_cast <int> (rand()) / (static_cast <int> (RAND_MAX / (max - min)));
}

void FscScene::LoadResources()
{
	//Textures
	//wr::TextureHandle ground_albedo =			m_texture_pool->LoadFromFile("resources/models/dreadroamer-fbx/textures/Ground_baseColor.png", true, true);
	//wr::TextureHandle ground_metal_roughness =	m_texture_pool->LoadFromFile("resources/models/tiki-treasure/Source/Pirates/Sky.png", false, true);
	//wr::TextureHandle ground_normal =			m_texture_pool->LoadFromFile("resources/models/tiki-treasure/Source/Pirates/Sky.png", false, true);

	// Models
	m_stalker_model = m_model_pool->LoadWithMaterials<wr::Vertex>(m_material_pool.get(), m_texture_pool.get(), "resources/models/dreadroamer-fbx/scene.gltf", false, &m_stalker_model_data);
	m_dragon_model = m_model_pool->LoadWithMaterials<wr::Vertex>(m_material_pool.get(), m_texture_pool.get(), "resources/models/blood-and-fire-gltf/scene.gltf", false, &m_dragon_model_data);
	m_environment_model = m_model_pool->LoadWithMaterials<wr::Vertex>(m_material_pool.get(), m_texture_pool.get(), "resources/models/fantasy_game_inn/scene.gltf", false, &m_environment_model_data);
	// Textures
	m_skybox = m_texture_pool->LoadFromFile("resources/materials/lookout_4k.hdr", false, false);
}

void FscScene::BuildScene(unsigned int width, unsigned int height, void* extra)
{

	m_camera = m_scene_graph->CreateChild<DebugCamera>(nullptr, 90.f, (float)width / (float)height);
	m_camera->SetPosition({ 550.f, 250.f, 75.f });
	m_camera->SetRotation({ -10_deg, 70_deg, 0_deg });
	m_camera->SetSpeed(150);
	m_camera->SetFrustumFar(1000.f);
	m_camera->SetFrustumNear(10.f);

	m_camera_spline_node = m_scene_graph->CreateChild<SplineNode>(nullptr, "Camera Spline", false);

	auto skybox = m_scene_graph->CreateChild<wr::SkyboxNode>(nullptr, m_skybox);

	// Geometry
	m_stalker_node = m_scene_graph->CreateChild<wr::MeshNode>(nullptr, m_stalker_model);
	m_stalker_node->SetPosition({ 22.f, 0.f, -220.f });
	m_stalker_node->SetRotation({ 0, 50_deg, 0 });

	m_dragon_node = m_scene_graph->CreateChild<wr::MeshNode>(nullptr, m_dragon_model);
	m_dragon_node->SetPosition({ 0.f, 160.f, 0.f });
	m_dragon_node->SetScale({ 0.25f, 0.25f, 0.25f });

	m_environment_node = m_scene_graph->CreateChild<wr::MeshNode>(nullptr, m_environment_model);
}

void FscScene::Update(float delta)
{
	m_camera->Update(delta);
	m_camera_spline_node->UpdateSplineNode(delta, m_camera);
}
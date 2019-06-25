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
	Scene(256, 200_mb, 200_mb),
	m_scene_model(nullptr),
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
	// Models
	m_scene_model = m_model_pool->LoadWithMaterials<wr::Vertex>(m_material_pool.get(), m_texture_pool.get(), "resources/models/dreadroamer/source/Zbot_Animation.fbx", false, &m_scene_model_data);

	// Textures
	m_skybox = m_texture_pool->LoadFromFile("resources/models/tiki-treasure/Source/Pirates/Sky.png", false, false);
}

void FscScene::BuildScene(unsigned int width, unsigned int height, void* extra)
{

	m_camera = m_scene_graph->CreateChild<DebugCamera>(nullptr, 90.f, (float)width / (float)height);
	m_camera->SetPosition({ 15, 55, 120 });
	m_camera->SetRotation({0, 15, 0});
	m_camera->SetSpeed(10);

	m_camera_spline_node = m_scene_graph->CreateChild<SplineNode>(nullptr, "Camera Spline", false);

	auto skybox = m_scene_graph->CreateChild<wr::SkyboxNode>(nullptr, m_skybox);

	// Geometry
	m_scene_node = m_scene_graph->CreateChild<wr::MeshNode>(nullptr, m_scene_model);
	m_scene_node->SetPosition({ 0, 0, 0 });
	m_scene_node->SetRotation({ 90_deg, 180_deg, 0 });
	//m_scene_node->SetScale({ -0.01f,0.01f,0.01f });
}

void FscScene::Update(float delta)
{
	m_camera->Update(delta);
	m_camera_spline_node->UpdateSplineNode(delta, m_camera);
}
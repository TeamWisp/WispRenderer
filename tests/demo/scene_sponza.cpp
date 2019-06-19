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

SponzaScene::SponzaScene() :
	Scene(256, 20_mb, 20_mb),
	m_sponza_model(nullptr),
	m_skybox({}),
	m_time(0)
{
	m_lights_path = "resources/sponza_lights.json";
}

void SponzaScene::LoadResources()
{
	// Models
	m_sponza_model = m_model_pool->LoadWithMaterials<wr::Vertex>(m_material_pool.get(), m_texture_pool.get(), "resources/models/sponza/sponza.obj");

	// Textures
	m_skybox = m_texture_pool->LoadFromFile("resources/materials/Barce_Rooftop_C_3k.hdr", false, false);
}

void SponzaScene::BuildScene(unsigned int width, unsigned int height, void* extra)
{
	m_camera = m_scene_graph->CreateChild<DebugCamera>(nullptr, 90.f, (float)width / (float)height);
	m_camera->SetPosition({ 0, 0, 0 });
	m_camera->SetSpeed(10);

	m_camera_spline_node = m_scene_graph->CreateChild<SplineNode>(nullptr, "Camera Spline", false);

	auto skybox = m_scene_graph->CreateChild<wr::SkyboxNode>(nullptr, m_skybox);

	// Geometry
	auto sponza = m_scene_graph->CreateChild<wr::MeshNode>(nullptr, m_sponza_model);
	sponza->SetPosition({ 0, -1, 0 });
	sponza->SetRotation({ 0, 90_deg, 0 });
	sponza->SetScale({ 0.01f,0.01f,0.01f });

	// Lights
	LoadLightsFromJSON();
}

void SponzaScene::Update(float delta)
{
	m_camera->Update(delta);
	m_camera_spline_node->UpdateSplineNode(delta, m_camera);
}
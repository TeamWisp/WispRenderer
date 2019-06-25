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

#include "scene_car.hpp"


CarScene::CarScene() :
	Scene(2560, 200_mb, 200_mb),
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

void CarScene::LoadResources()
{
	// Models
	m_scene_model = m_model_pool->Load<wr::Vertex>(m_material_pool.get(), m_texture_pool.get(), "resources/models/robo-obj-pose4/source/d2f0cff60afc40f5afe79156ec7db657.obj");


	wr::TextureHandle scene_albedo = m_texture_pool->LoadFromFile("resources/models/robo-obj-pose4/textures/Texture_1K.jpg", true, true);
	wr::TextureHandle scene_normal = m_texture_pool->LoadFromFile("resources/models/robo-obj-pose4/textures/LP_BodyNormalsMap_1K.jpg", false, true);
	wr::TextureHandle scene_roughness = m_texture_pool->LoadFromFile("resources/models/robo-obj-pose4/textures/98a91fc2e52c4db6a4be147a471e98ca.jpeg", false, true);
	wr::TextureHandle scene_emissive = m_texture_pool->LoadFromFile("resources/models/robo-obj-pose4/textures/6bf488141a8a487599953582478eca36.jpeg", false, true);


	auto mat = m_material_pool->Create(m_texture_pool.get());
	wr::Material* mat_internal = m_material_pool->GetMaterial(mat);
	mat_internal->SetTexture(wr::TextureType::ALBEDO, scene_albedo);
	mat_internal->SetTexture(wr::TextureType::ROUGHNESS, scene_roughness);
	mat_internal->SetTexture(wr::TextureType::NORMAL, scene_normal);
	mat_internal->SetTexture(wr::TextureType::EMISSIVE, scene_emissive);

	mat_internal->SetConstant<wr::MaterialConstant::METALLIC>(0.5f);
	mat_internal->SetConstant<wr::MaterialConstant::EMISSIVE_MULTIPLIER>(1.f);
	m_scene_material = mat;

	// Textures
	m_skybox = m_texture_pool->LoadFromFile("resources/materials/Barce_Rooftop_C_3k.hdr", false, false);
}

void CarScene::BuildScene(unsigned int width, unsigned int height, void* extra)
{

	m_camera = m_scene_graph->CreateChild<DebugCamera>(nullptr, 90.f, (float)width / (float)height);
	m_camera->SetPosition({ 15, 55, 120 });
	m_camera->SetRotation({ 0, 15, 0 });
	m_camera->SetSpeed(10);

	m_camera_spline_node = m_scene_graph->CreateChild<SplineNode>(nullptr, "Camera Spline", false);

	auto skybox = m_scene_graph->CreateChild<wr::SkyboxNode>(nullptr, m_skybox);

	// Geometry
	m_scene_node = m_scene_graph->CreateChild<wr::MeshNode>(nullptr, m_scene_model);
	m_scene_node->AddMaterial(m_scene_material);
	m_scene_node->SetPosition({ 0, 0, 0 });
	m_scene_node->SetRotation({ 90_deg, 180_deg, 0 });
	//m_scene_node->SetScale({ -0.01f,0.01f,0.01f });
}

void CarScene::Update(float delta)
{
	m_camera->Update(delta);
	m_camera_spline_node->UpdateSplineNode(delta, m_camera);
}
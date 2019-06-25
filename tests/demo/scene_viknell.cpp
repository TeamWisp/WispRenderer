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
	Scene(256, 400_mb, 400_mb),
	m_skybox({}),
	m_time(0)
{
	m_lights_path = "resources/viknell_lights.json";
	m_models_path = "resources/viknell_models.json";
}

void ViknellScene::LoadResources()
{
	//pipes, ground, walls, walls_low1, ground, pipes, decor.

	// Models
	m_car_model = m_model_pool->Load<wr::Vertex>(m_material_pool.get(), m_texture_pool.get(), "resources/models/cyber_warrior/source/warrior.fbx");
	m_cube_model = m_model_pool->Load<wr::Vertex>(m_material_pool.get(), m_texture_pool.get(), "resources/models/metro_tube/source/metro_block_low.fbx");
	//m_walls_model = m_model_pool->Load<wr::Vertex>(m_material_pool.get(), m_texture_pool.get(), "resources/models/book_scan/source/CopticBookModel.fbx");
	m_coin_model = m_model_pool->Load<wr::Vertex>(m_material_pool.get(), m_texture_pool.get(), "resources/models/enemy_creature/MythCreature.fbx");

	// Textures
	m_skybox = m_texture_pool->LoadFromFile("resources/materials/studio001.hdr", false, false);

	wr::TextureHandle mat_2_albedo = m_texture_pool->LoadFromFile("resources/models/cyber_warrior/textures/TM.png", true, true);
	wr::TextureHandle mat_2_normal = m_texture_pool->LoadFromFile("resources/models/cyber_warrior/textures/NM.png", false, true);
	wr::TextureHandle mat_2_rough = m_texture_pool->LoadFromFile("resources/models/cyber_warrior/textures/Rough.png", false, true);
	wr::TextureHandle mat_2_metal = m_texture_pool->LoadFromFile("resources/models/cyber_warrior/textures/Metal.png", false, true);
	wr::TextureHandle mat_2_emiss = m_texture_pool->LoadFromFile("resources/models/cyber_warrior/textures/EMI.png", false, true);
	wr::TextureHandle mat_2_ao = m_texture_pool->LoadFromFile("resources/models/cyber_warrior/textures/normal_occlusion2.png", false, true);

	wr::TextureHandle ground_albedo = m_texture_pool->LoadFromFile("resources/models/metro_tube/textures/ground_part_albedo.jpeg", true, true);
	wr::TextureHandle ground_normal = m_texture_pool->LoadFromFile("resources/models/metro_tube/textures/ground_part_normal.jpeg", false, true);
	wr::TextureHandle ground_metallic = m_texture_pool->LoadFromFile("resources/models/metro_tube/textures/ground_part_metallic.jpeg", false, true);
	wr::TextureHandle ground_roughness = m_texture_pool->LoadFromFile("resources/models/metro_tube/textures/ground_part_roughness.jpeg", false, true);

	wr::TextureHandle decor_albedo = m_texture_pool->LoadFromFile("resources/models/metro_tube/textures/metro_decor_albedo.jpeg", true, true);
	wr::TextureHandle decor_normal = m_texture_pool->LoadFromFile("resources/models/metro_tube/textures/metro_decor_normal.jpeg", false, true);
	wr::TextureHandle decor_metallic = m_texture_pool->LoadFromFile("resources/models/metro_tube/textures/metro_decor_metallic.jpeg", false, true);
	wr::TextureHandle decor_roughness = m_texture_pool->LoadFromFile("resources/models/metro_tube/textures/metro_decor_roughness.jpeg", false, true);
	wr::TextureHandle decor_ao = m_texture_pool->LoadFromFile("resources/models/metro_tube/textures/metro_decor_AO.jpeg", false, true);
	wr::TextureHandle decor_emissive = m_texture_pool->LoadFromFile("resources/models/metro_tube/textures/metro_decor_emissive.jpeg", false, true);

	wr::TextureHandle pipes_albedo = m_texture_pool->LoadFromFile("resources/models/metro_tube/textures/pipes_albedo.jpeg", true, true);
	wr::TextureHandle pipes_normal = m_texture_pool->LoadFromFile("resources/models/metro_tube/textures/pipes_normal.jpeg", false, true);
	wr::TextureHandle pipes_metallic = m_texture_pool->LoadFromFile("resources/models/metro_tube/textures/pipes_metallic.jpeg", false, true);
	wr::TextureHandle pipes_roughness = m_texture_pool->LoadFromFile("resources/models/metro_tube/textures/pipes_roughness.jpeg", false, true);

	wr::TextureHandle walls_albedo = m_texture_pool->LoadFromFile("resources/models/metro_tube/textures/walls_albedo.jpeg", true, true);
	wr::TextureHandle walls_normal = m_texture_pool->LoadFromFile("resources/models/metro_tube/textures/walls_normal.jpeg", false, true);
	wr::TextureHandle walls_metallic = m_texture_pool->LoadFromFile("resources/models/metro_tube/textures/walls_metallic.jpeg", false, true);
	wr::TextureHandle walls_roughness = m_texture_pool->LoadFromFile("resources/models/metro_tube/textures/walls_roughness.jpeg", false, true);

	//Spider
	wr::TextureHandle abdomen_albedo = m_texture_pool->LoadFromFile("resources/models/enemy_creature/textures/Abdomen_albedo.jpeg", true, true);
	wr::TextureHandle abdomen_normal = m_texture_pool->LoadFromFile("resources/models/enemy_creature/textures/Abdomen_Normal.jpeg", false, true);
	wr::TextureHandle abdomen_emissive = m_texture_pool->LoadFromFile("resources/models/enemy_creature/textures/Abdomen_Emissive.jpeg", false, true);
	wr::TextureHandle abdomen_metallic = m_texture_pool->LoadFromFile("resources/models/enemy_creature/textures/Abdomen_Metallic.jpeg", false, true);
	wr::TextureHandle abdomen_roughness = m_texture_pool->LoadFromFile("resources/models/enemy_creature/textures/Abdomen_Roughness.jpeg", false, true);

	wr::TextureHandle eye_albedo = m_texture_pool->LoadFromFile("resources/models/enemy_creature/textures/Eyes_Albedo.jpeg", false, true);
	wr::TextureHandle eye_emissive = m_texture_pool->LoadFromFile("resources/models/enemy_creature/textures/Eyes_Emissive.jpeg", false, true);
	wr::TextureHandle eye_normal = m_texture_pool->LoadFromFile("resources/models/enemy_creature/textures/Eyes_Normal.jpg", false, true);
	wr::TextureHandle eye_roughness = m_texture_pool->LoadFromFile("resources/models/enemy_creature/textures/Eyes_Roughness.jpg", false, true);

	wr::TextureHandle hair_albedo = m_texture_pool->LoadFromFile("resources/models/enemy_creature/textures/Hair_Albedo.png", false, true);
	wr::TextureHandle hair_normal = m_texture_pool->LoadFromFile("resources/models/enemy_creature/textures/Hair_Normal.jpg", false, true);
	wr::TextureHandle hair_roughness = m_texture_pool->LoadFromFile("resources/models/enemy_creature/textures/Hair_Roughness.jpg", false, true);

	wr::TextureHandle lowtorso_albedo = m_texture_pool->LoadFromFile("resources/models/enemy_creature/textures/LowerTorso_Albedo.jpeg", false, true);
	wr::TextureHandle lowtorso_normal = m_texture_pool->LoadFromFile("resources/models/enemy_creature/textures/LowerTorso_Normal.jpeg", false, true);
	wr::TextureHandle lowtorso_roughness = m_texture_pool->LoadFromFile("resources/models/enemy_creature/textures/LowerTorso_Roughness.jpeg", false, true);

	wr::TextureHandle uptorso_albedo = m_texture_pool->LoadFromFile("resources/models/enemy_creature/textures/UpperTorso_Albedo.jpeg", false, true);
	wr::TextureHandle uptorso_emissive = m_texture_pool->LoadFromFile("resources/models/enemy_creature/textures/UpperTorso_Emissive.jpeg", false, true);
	wr::TextureHandle uptorso_metallic = m_texture_pool->LoadFromFile("resources/models/enemy_creature/textures/UpperTorso_Metallic.jpeg", false, true);
	wr::TextureHandle uptorso_normal = m_texture_pool->LoadFromFile("resources/models/enemy_creature/textures/UpperTorso_Normal.jpeg", false, true);
	wr::TextureHandle uptorso_roughness = m_texture_pool->LoadFromFile("resources/models/enemy_creature/textures/UpperTorso_Roughness.jpeg", false, true);



	//Mat 2, character
	m_material_2 = m_material_pool->Create(m_texture_pool.get());
	wr::Material* mat_2_int = m_material_pool->GetMaterial(m_material_2);
	mat_2_int->SetTexture(wr::TextureType::ALBEDO, mat_2_albedo);
	mat_2_int->SetTexture(wr::TextureType::NORMAL, mat_2_normal);
	mat_2_int->SetTexture(wr::TextureType::ROUGHNESS, mat_2_rough);
	mat_2_int->SetTexture(wr::TextureType::METALLIC, mat_2_metal);
	mat_2_int->SetTexture(wr::TextureType::EMISSIVE, mat_2_emiss);
	mat_2_int->SetTexture(wr::TextureType::AO, mat_2_ao);
	mat_2_int->SetConstant<wr::MaterialConstant::EMISSIVE_MULTIPLIER>(100.0f);


	//Mat 1, bridge
	m_material_1 = m_material_pool->Create(m_texture_pool.get());
	wr::Material* pipes_internal = m_material_pool->GetMaterial(m_material_1);
	pipes_internal->SetTexture(wr::TextureType::ALBEDO, pipes_albedo);
	pipes_internal->SetTexture(wr::TextureType::NORMAL, pipes_normal);
	pipes_internal->SetTexture(wr::TextureType::METALLIC, pipes_metallic);
	pipes_internal->SetTexture(wr::TextureType::ROUGHNESS, pipes_roughness);

	//Mat 3, bridge
	m_material_3 = m_material_pool->Create(m_texture_pool.get());
	wr::Material* ground_int = m_material_pool->GetMaterial(m_material_3);
	ground_int->SetTexture(wr::TextureType::ALBEDO, ground_albedo);
	ground_int->SetTexture(wr::TextureType::NORMAL, ground_normal);
	ground_int->SetTexture(wr::TextureType::METALLIC, ground_metallic);
	ground_int->SetTexture(wr::TextureType::ROUGHNESS, ground_roughness);


	//Mat 4, bridge
	m_material_4 = m_material_pool->Create(m_texture_pool.get());
	wr::Material* walls_int = m_material_pool->GetMaterial(m_material_4);
	walls_int->SetTexture(wr::TextureType::ALBEDO, walls_albedo);
	walls_int->SetTexture(wr::TextureType::NORMAL, walls_normal);
	walls_int->SetTexture(wr::TextureType::METALLIC, walls_metallic);
	walls_int->SetTexture(wr::TextureType::ROUGHNESS, walls_roughness);

	
	//Mat 5, bridge
	m_material_5 = m_material_pool->Create(m_texture_pool.get());
	wr::Material* decor_int = m_material_pool->GetMaterial(m_material_5);
	decor_int->SetTexture(wr::TextureType::ALBEDO, decor_albedo);
	decor_int->SetTexture(wr::TextureType::NORMAL, decor_normal);
	decor_int->SetTexture(wr::TextureType::METALLIC, decor_metallic);
	decor_int->SetTexture(wr::TextureType::ROUGHNESS, decor_roughness);
	decor_int->SetTexture(wr::TextureType::AO, decor_ao);
	decor_int->SetTexture(wr::TextureType::EMISSIVE, decor_emissive);
	decor_int->SetConstant<wr::MaterialConstant::EMISSIVE_MULTIPLIER>(25.0f);


	// Enemy
	m_abdomen = m_material_pool->Create(m_texture_pool.get());
	wr::Material* abd_int = m_material_pool->GetMaterial(m_abdomen);
	abd_int->SetTexture(wr::TextureType::ALBEDO, abdomen_albedo);
	abd_int->SetTexture(wr::TextureType::NORMAL, abdomen_normal);
	abd_int->SetTexture(wr::TextureType::METALLIC, abdomen_metallic);
	abd_int->SetTexture(wr::TextureType::ROUGHNESS, abdomen_roughness);
	abd_int->SetTexture(wr::TextureType::EMISSIVE, abdomen_emissive);
	abd_int->SetConstant<wr::MaterialConstant::EMISSIVE_MULTIPLIER>(2.0f);

	m_eyes = m_material_pool->Create(m_texture_pool.get());
	wr::Material* eyes_int = m_material_pool->GetMaterial(m_eyes);
	eyes_int->SetTexture(wr::TextureType::ALBEDO, eye_albedo);
	eyes_int->SetTexture(wr::TextureType::NORMAL, eye_normal);
	eyes_int->SetTexture(wr::TextureType::ROUGHNESS, eye_roughness);
	eyes_int->SetTexture(wr::TextureType::EMISSIVE, eye_emissive);
	eyes_int->SetConstant<wr::MaterialConstant::EMISSIVE_MULTIPLIER>(2.0f);
	eyes_int->SetConstant<wr::MaterialConstant::METALLIC>(0.0f);

	m_hair = m_material_pool->Create(m_texture_pool.get());
	wr::Material* hair_int = m_material_pool->GetMaterial(m_hair);
	hair_int->SetTexture(wr::TextureType::ALBEDO, hair_albedo);
	hair_int->SetTexture(wr::TextureType::NORMAL, hair_normal);
	hair_int->SetTexture(wr::TextureType::ROUGHNESS, hair_roughness);
	hair_int->SetConstant<wr::MaterialConstant::METALLIC>(0.0f);

	m_lowerTorso = m_material_pool->Create(m_texture_pool.get());
	wr::Material* low_torso_int = m_material_pool->GetMaterial(m_lowerTorso);
	low_torso_int->SetTexture(wr::TextureType::ALBEDO, lowtorso_albedo);
	low_torso_int->SetTexture(wr::TextureType::NORMAL, lowtorso_normal);
	low_torso_int->SetTexture(wr::TextureType::ROUGHNESS, lowtorso_roughness);
	low_torso_int->SetConstant<wr::MaterialConstant::METALLIC>(0.0f);

	m_upperTorso = m_material_pool->Create(m_texture_pool.get());
	wr::Material* up_torso_int = m_material_pool->GetMaterial(m_upperTorso);
	up_torso_int->SetTexture(wr::TextureType::ALBEDO, uptorso_albedo);
	up_torso_int->SetTexture(wr::TextureType::NORMAL, uptorso_normal);
	up_torso_int->SetTexture(wr::TextureType::ROUGHNESS, uptorso_roughness);
	up_torso_int->SetTexture(wr::TextureType::METALLIC, uptorso_metallic);
	up_torso_int->SetTexture(wr::TextureType::EMISSIVE, uptorso_emissive);
	up_torso_int->SetConstant<wr::MaterialConstant::EMISSIVE_MULTIPLIER>(1.0f);
}

void ViknellScene::BuildScene(unsigned int width, unsigned int height, void* extra)
{
	m_camera = m_scene_graph->CreateChild<DebugCamera>(nullptr, 90.f, (float)width / (float)height);
	m_camera->SetPosition({ 0, 0, 2 });
	m_camera->SetSpeed(50);

	m_camera_spline_node = m_scene_graph->CreateChild<SplineNode>(nullptr, "Camera Spline", false);

	auto skybox = m_scene_graph->CreateChild<wr::SkyboxNode>(nullptr, m_skybox);

	// Geometry

	m_car_node = m_scene_graph->CreateChild<wr::MeshNode>(nullptr, m_car_model);
	m_car_node->SetMaterials({ m_material_2 });

	m_walls_node = m_scene_graph->CreateChild<wr::MeshNode>(nullptr, m_cube_model);
	m_walls_node->SetMaterials({ m_material_1, m_material_3, m_material_4, m_material_4, m_material_3, m_material_1, m_material_5 });

	auto enemy = m_scene_graph->CreateChild<wr::MeshNode>(nullptr, m_coin_model);
	enemy->SetMaterials({ m_hair , m_lowerTorso , m_eyes , m_hair , m_upperTorso , m_upperTorso , m_abdomen , m_upperTorso });

	LoadLightsFromJSON();
}

void ViknellScene::Update(float delta)
{
	m_camera->Update(delta);
	m_camera_spline_node->UpdateSplineNode(delta, m_camera);
}
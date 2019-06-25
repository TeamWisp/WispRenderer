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

#include "scene_var_roughness.hpp"

static constexpr int num_divisions = 8;

VarRoughnessScene::VarRoughnessScene():
	Scene(256, 20_mb, 20_mb),
	m_cube_model(nullptr),
	m_skybox({}),
	m_time(0) { }


inline float RandRange(float min, float max)
{
	return min + static_cast<float> (rand()) / (static_cast<float> (RAND_MAX / (max - min)));
}

inline float RandRangeI(float min, float max)
{
	return min + static_cast<int> (rand()) / (static_cast<int> (RAND_MAX / (max - min)));
}

VarRoughnessScene::~VarRoughnessScene()
{
	if (m_cube_model)
	{
		m_model_pool->Destroy(m_cube_model);

		for (auto& material : m_roughness_materials)
		{
			m_material_pool->DestroyMaterial(material);
		}
	}
}

void VarRoughnessScene::LoadResources()
{
	// Models
	m_cube_model = m_model_pool->Load<wr::Vertex>(m_material_pool.get(), m_texture_pool.get(), "resources/models/cube.fbx");

	// Textures
	m_skybox = m_texture_pool->LoadFromFile("resources/materials/Arches_E_PineTree_3k.hdr", false, false);

	// Materials
	for (auto i = 0; i < num_divisions; i++)
	{
		auto mat = m_material_pool->Create(m_texture_pool.get());
		wr::Material *mat_internal = m_material_pool->GetMaterial(mat);
		mat_internal->SetConstant<wr::MaterialConstant::ROUGHNESS>(float(i) / (num_divisions - 1));
		mat_internal->SetConstant<wr::MaterialConstant::METALLIC>(1);
		mat_internal->SetConstant<wr::MaterialConstant::EMISSIVE_MULTIPLIER>(0);
		mat_internal->SetConstant<wr::MaterialConstant::COLOR>({ 1, 1, 1 });
		m_roughness_materials.push_back(mat);
	}
}

void VarRoughnessScene::BuildScene(unsigned int width, unsigned int height, void* extra)
{
	auto& phys_engine = *reinterpret_cast<phys::PhysicsEngine*>(extra);

	m_camera = m_scene_graph->CreateChild<DebugCamera>(nullptr, 90.f, (float)width / (float)height);
	m_camera->SetPosition({ 3.576, 3.657, -5.008 });
	m_camera->SetRotation({ -14.043_deg, 60.378_deg, 0.905_deg });
	m_camera->SetSpeed(1);

	m_camera_spline_node = m_scene_graph->CreateChild<SplineNode>(nullptr, "Camera Spline", false);

	auto skybox = m_scene_graph->CreateChild<wr::SkyboxNode>(nullptr, m_skybox);

	// Left Ballfall
	for (auto i = 0; i < num_divisions; i++) {
		auto floor = m_scene_graph->CreateChild<wr::MeshNode>(nullptr, m_cube_model);
		floor->SetPosition({ -1.f, 1.f, -7.f - 2 * i });
		floor->SetScale({ 3.f, 0.1f, 1.f });
		floor->AddMaterial(m_roughness_materials[i]);
		auto cube = m_scene_graph->CreateChild<wr::MeshNode>(nullptr, m_cube_model);
		cube->SetPosition({ -4.f, .9f + .8f, -7.f -2 * i });
		cube->SetScale({ .8f, 0.8f, 0.8f });		
		cube->AddMaterial(m_roughness_materials[i]);
	}

}

void VarRoughnessScene::Update(float delta)
{
	m_camera->Update(delta);
	m_camera_spline_node->UpdateSplineNode(delta, m_camera);
}
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

void SponzaScene::Update()
{
	m_camera->Update(ImGui::GetIO().DeltaTime);
	m_camera_spline_node->UpdateSplineNode(ImGui::GetIO().DeltaTime, m_camera);
}
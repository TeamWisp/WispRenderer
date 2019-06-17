#pragma once

#include "scene_shadows_transparency.hpp"

TransparencyScene::TransparencyScene() :
	Scene(256, 2_mb, 2_mb),
	m_plant_model(nullptr),
	m_plant_material(),
	m_gray_material(),
	m_env_map({}),
	m_time(0)
{
	m_lights_path = "resources/shadow_transparency_lights.json";
}

void TransparencyScene::LoadResources()
{
	// Models
	m_plant_model = m_model_pool->Load<wr::Vertex>(m_material_pool.get(), m_texture_pool.get(), "resources/models/plant/tropical_2.fbx");
	
	// Textures
	wr::TextureHandle leaf_albedo = m_texture_pool->LoadFromFile("resources/models/plant/tropical_plant.png", true, true);

	m_env_map = m_texture_pool->LoadFromFile("resources/materials/Circus_Backstage_3k.hdr", false, false);

	// Materials
	m_plant_material = m_material_pool->Create(m_texture_pool.get());
	wr::Material* plant_internal = m_material_pool->GetMaterial(m_plant_material);

	plant_internal->SetTexture(wr::TextureType::ALBEDO, leaf_albedo);
	plant_internal->SetConstant<wr::MaterialConstant::IS_ALPHA_MASKED>(true);
	plant_internal->SetConstant<wr::MaterialConstant::IS_DOUBLE_SIDED>(true);
	plant_internal->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.99f);
	plant_internal->SetConstant<wr::MaterialConstant::METALLIC>(0.01f);

	//Gray mat
	m_gray_material = m_material_pool->Create(m_texture_pool.get());
	wr::Material* gray_internal = m_material_pool->GetMaterial(m_gray_material);

	gray_internal->SetConstant<wr::MaterialConstant::COLOR>({ 0.5f, 0.5f, 0.5f });
	gray_internal->SetConstant<wr::MaterialConstant::ROUGHNESS>(0.99f);
	gray_internal->SetConstant<wr::MaterialConstant::METALLIC>(0.01f);
}

void TransparencyScene::BuildScene(unsigned int width, unsigned int height, void* extra)
{
	auto& phys_engine = *reinterpret_cast<phys::PhysicsEngine*>(extra);

	m_camera = m_scene_graph->CreateChild<DebugCamera>(nullptr, 90.f, (float)width / (float)height);
	m_camera->SetPosition({ 0, 0, 2 });
	m_camera->SetSpeed(10);

	auto skybox = m_scene_graph->CreateChild<wr::SkyboxNode>(nullptr, m_env_map);
	m_plant_node = m_scene_graph->CreateChild<wr::MeshNode>(nullptr, m_plant_model);

	std::vector<wr::MaterialHandle> mats(3);
	mats[0] = m_gray_material;
	mats[1] = m_plant_material;
	mats[2] = m_plant_material;

	m_plant_node->SetMaterials(mats);
	m_plant_node->SetScale({ 0.1f, 0.1f, 0.1f });

	// Lights
	auto point_light_0 = m_scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::DIRECTIONAL, DirectX::XMVECTOR{ 3.0f, 3.0f, 3.0f });
	point_light_0->SetRotation({ 220.0_deg, 0.0f, 0.0f });
}

void TransparencyScene::Update()
{
	m_camera->Update(ImGui::GetIO().DeltaTime);
}
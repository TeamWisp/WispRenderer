//#pragma once
//
//#include "spheres_scene.hpp"
//
//SpheresScene::SpheresScene() :
//	Scene(256, 2_mb, 2_mb),
//	m_sphere_model(nullptr),
//	m_skybox({})
//{
//
//}
//
//void SpheresScene::LoadResources()
//{
//	// Models
//	m_sphere_model = m_model_pool->Load<wr::Vertex>(m_material_pool.get(), m_texture_pool.get(), "resources/models/sphere.fbx");
//
//	// Textures
//	m_skybox = m_texture_pool->LoadFromFile("resources/materials/Circus_Backstage_3k.hdr", false, false);
//	m_flat_normal = m_texture_pool->LoadFromFile("resources/materials/flat_normal.png", false, false);
//}
//
//void SpheresScene::BuildScene(unsigned int width, unsigned int height)
//{
//	auto camera = m_scene_graph->CreateChild<wr::CameraNode>(nullptr, (float)width / height);
//	camera->SetFov(45.f);
//	camera->SetPosition({ 0.0f, 0.0f, 30.f });
//
//	m_scene_graph->CreateChild<wr::SkyboxNode>(nullptr, m_skybox);
//
//	std::shared_ptr<wr::MeshNode> spheres[49];
//	for (uint32_t i = 0; i <= 6; ++i)
//	{
//		for (uint32_t j = 0; j <= 6; ++j)
//		{
//			spheres[i * 7 + j] = m_scene_graph->CreateChild<wr::MeshNode>(nullptr, m_sphere_model);
//
//			float x_pos = Lerp(10.0f, -10.0f, (float)i / 6.f);
//			float y_pos = Lerp(10.0f, -10.0f, (float)j / 6.f);
//
//			spheres[i * 7 + j]->SetPosition({ x_pos, y_pos, 0.0f, 0.0f });
//			spheres[i * 7 + j]->SetRotation({ 90.0_deg, 0.0_deg, 0.0_deg });
//
//			float roughness = Lerp(1.0f, 0.0f, (float)i / 6);
//			float metallic = Lerp(1.0f, 0.0f, (float)j / 6);
//
//			//Create new material
//			wr::MaterialHandle mat = m_material_pool->Create(m_texture_pool.get());
//			auto mat_internal = mat.m_pool->GetMaterial(mat);
//
//			mat_internal->SetConstant<wr::MaterialConstant::COLOR>({ 1, 0, 0 });
//			mat_internal->SetTexture(wr::TextureType::NORMAL, m_flat_normal);
//			mat_internal->SetConstant<wr::MaterialConstant::ROUGHNESS>(roughness);
//			mat_internal->SetConstant<wr::MaterialConstant::METALLIC>(metallic);
//
//			spheres[i * 7 + j]->SetMaterials({ mat });
//		}
//	}
//
//	auto dir_light = m_scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::DIRECTIONAL, DirectX::XMVECTOR{ 0, 1, 0 });
//	dir_light->SetDirectional({ 136._deg, 0, 0 }, { 4, 4, 4 });
//}
//
//void SpheresScene::Update()
//{
//
//}
//
//float SpheresScene::Lerp(float v0, float v1, float t)
//{
//	return (1.0f - t) * v0 + t * v1;
//}
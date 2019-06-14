#pragma once

#include "scene_emibl.hpp"

EmiblScene::EmiblScene() :
	Scene(256, 2_mb, 2_mb),
	m_cube_model(nullptr),
	m_plane_model(nullptr),
	m_material_knob_model(nullptr),
	m_rusty_metal_material(),
	m_knob_material(),
	m_skybox({})
{

}

void EmiblScene::LoadResources()
{
	// Textures
	wr::TextureHandle metal_splotchy_albedo = m_texture_pool->LoadFromFile("resources/materials/metal-splotchy-albedo.png", true, true);
	wr::TextureHandle metal_splotchy_normal = m_texture_pool->LoadFromFile("resources/materials/metal-splotchy-normal-dx.png", false, true);
	wr::TextureHandle metal_splotchy_roughness = m_texture_pool->LoadFromFile("resources/materials/metal-splotchy-rough.png", false, true);
	wr::TextureHandle metal_splotchy_metallic = m_texture_pool->LoadFromFile("resources/materials/metal-splotchy-metal.png", false, true);

	wr::TextureHandle mahogfloor_albedo = m_texture_pool->LoadFromFile("resources/materials/harsh_bricks/albedo.png", true, true);
	wr::TextureHandle mahogfloor_normal = m_texture_pool->LoadFromFile("resources/materials/harsh_bricks/normal.png", false, true);
	wr::TextureHandle mahogfloor_roughness = m_texture_pool->LoadFromFile("resources/materials/harsh_bricks/roughness.png", false, true);
	wr::TextureHandle mahogfloor_metallic = m_texture_pool->LoadFromFile("resources/materials/harsh_bricks/metallic.png", false, true);
	wr::TextureHandle mahogfloor_ao = m_texture_pool->LoadFromFile("resources/materials/harsh_bricks/ao.png", false, true);

	wr::TextureHandle red_black_albedo = m_texture_pool->LoadFromFile("resources/materials/dragon_egg/albedo.png", true, true);
	wr::TextureHandle red_black_normal = m_texture_pool->LoadFromFile("resources/materials/dragon_egg/normal.png", false, true);
	wr::TextureHandle red_black_roughness = m_texture_pool->LoadFromFile("resources/materials/dragon_egg/roughness.png", false, true);
	wr::TextureHandle red_black_metallic = m_texture_pool->LoadFromFile("resources/materials/dragon_egg/metallic.png", false, true);

	wr::TextureHandle metal_albedo = m_texture_pool->LoadFromFile("resources/materials/greasy_pan/albedo.png", true, true);
	wr::TextureHandle metal_normal = m_texture_pool->LoadFromFile("resources/materials/greasy_pan/normal.png", false, true);
	wr::TextureHandle metal_roughness = m_texture_pool->LoadFromFile("resources/materials/greasy_pan/roughness.png", false, true);
	wr::TextureHandle metal_metallic = m_texture_pool->LoadFromFile("resources/materials/greasy_pan/metallic.png", false, true);

	wr::TextureHandle brick_tiles_albedo = m_texture_pool->LoadFromFile("resources/materials/brick_tiles/albedo.png", true, true);
	wr::TextureHandle brick_tiles_normal = m_texture_pool->LoadFromFile("resources/materials/brick_tiles/normal.png", false, true);
	wr::TextureHandle brick_tiles_roughness = m_texture_pool->LoadFromFile("resources/materials/brick_tiles/roughness.png", false, true);
	wr::TextureHandle brick_tiles_metallic = m_texture_pool->LoadFromFile("resources/materials/brick_tiles/metallic.png", false, true);

	wr::TextureHandle leather_albedo = m_texture_pool->LoadFromFile("resources/materials/leather_with_metal/albedo.png", true, true);
	wr::TextureHandle leather_normal = m_texture_pool->LoadFromFile("resources/materials/leather_with_metal/normal.png", false, true);
	wr::TextureHandle leather_roughness = m_texture_pool->LoadFromFile("resources/materials/leather_with_metal/roughness.png", false, true);
	wr::TextureHandle leather_metallic = m_texture_pool->LoadFromFile("resources/materials/leather_with_metal/metallic.png", false, true);

	wr::TextureHandle blue_tiles_albedo = m_texture_pool->LoadFromFile("resources/materials/blue_tiles/albedo.png", true, true);
	wr::TextureHandle blue_tiles_normal = m_texture_pool->LoadFromFile("resources/materials/blue_tiles/normal.png", false, true);
	wr::TextureHandle blue_tiles_roughness = m_texture_pool->LoadFromFile("resources/materials/blue_tiles/roughness.png", false, true);
	wr::TextureHandle blue_tiles_metallic = m_texture_pool->LoadFromFile("resources/materials/blue_tiles/metallic.png", false, true);

	wr::TextureHandle gold_albedo = m_texture_pool->LoadFromFile("resources/materials/gold_scuffed/albedo.png", true, true);
	wr::TextureHandle gold_normal = m_texture_pool->LoadFromFile("resources/materials/gold_scuffed/normal.png", false, true);
	wr::TextureHandle gold_roughness = m_texture_pool->LoadFromFile("resources/materials/gold_scuffed/roughness.png", false, true);
	wr::TextureHandle gold_metallic = m_texture_pool->LoadFromFile("resources/materials/gold_scuffed/metallic.png", false, true);

	wr::TextureHandle marble_albedo = m_texture_pool->LoadFromFile("resources/materials/marble_speckled/albedo.png", true, true);
	wr::TextureHandle marble_normal = m_texture_pool->LoadFromFile("resources/materials/marble_speckled/normal.png", false, true);
	wr::TextureHandle marble_roughness = m_texture_pool->LoadFromFile("resources/materials/marble_speckled/roughness.png", false, true);
	wr::TextureHandle marble_metallic = m_texture_pool->LoadFromFile("resources/materials/marble_speckled/metallic.png", false, true);


	wr::TextureHandle floreal_tiles_albedo = m_texture_pool->LoadFromFile("resources/materials/floreal_tiles/albedo.png", true, true);
	wr::TextureHandle floreal_tiles_normal = m_texture_pool->LoadFromFile("resources/materials/floreal_tiles/normal.png", false, true);
	wr::TextureHandle floreal_tiles_roughness = m_texture_pool->LoadFromFile("resources/materials/floreal_tiles/roughness.png", false, true);
	wr::TextureHandle floreal_tiles_metallic = m_texture_pool->LoadFromFile("resources/materials/floreal_tiles/metallic.png", false, true);

	wr::TextureHandle bw_tiles_albedo = m_texture_pool->LoadFromFile("resources/materials/bw_tiles_gold_lining/albedo.png", true, true);
	wr::TextureHandle bw_tiles_normal = m_texture_pool->LoadFromFile("resources/materials/bw_tiles_gold_lining/normal.png", false, true);
	wr::TextureHandle bw_tiles_roughness = m_texture_pool->LoadFromFile("resources/materials/bw_tiles_gold_lining/roughness.png", false, true);
	wr::TextureHandle bw_tiles_metallic = m_texture_pool->LoadFromFile("resources/materials/bw_tiles_gold_lining/metallic.png", false, true);
	wr::TextureHandle bw_tiles_emissive = m_texture_pool->LoadFromFile("resources/materials/bw_tiles_gold_lining/emissive.png", true, true);

	m_skybox = m_texture_pool->LoadFromFile("resources/materials/Arches_E_PineTree_3k.hdr", false, false);

	{
		// Create Material
		m_rusty_metal_material = m_material_pool->Create(m_texture_pool.get());

		wr::Material* rusty_metal_internal = m_material_pool->GetMaterial(m_rusty_metal_material);

		rusty_metal_internal->SetTexture(wr::TextureType::ALBEDO, metal_splotchy_albedo);
		rusty_metal_internal->SetTexture(wr::TextureType::NORMAL, metal_splotchy_normal);
		rusty_metal_internal->SetTexture(wr::TextureType::ROUGHNESS, metal_splotchy_roughness);
		rusty_metal_internal->SetTexture(wr::TextureType::METALLIC, metal_splotchy_metallic);

		// Create Material
		m_material_handles[0] = m_material_pool->Create(m_texture_pool.get());

		wr::Material* mahogfloor_material_internal = m_material_pool->GetMaterial(m_material_handles[0]);

		mahogfloor_material_internal->SetTexture(wr::TextureType::ALBEDO, mahogfloor_albedo);
		mahogfloor_material_internal->SetTexture(wr::TextureType::NORMAL, mahogfloor_normal);
		mahogfloor_material_internal->SetTexture(wr::TextureType::ROUGHNESS, mahogfloor_roughness);
		mahogfloor_material_internal->SetTexture(wr::TextureType::METALLIC, mahogfloor_metallic);
		mahogfloor_material_internal->SetTexture(wr::TextureType::AO, mahogfloor_ao);

		// Create Material
		m_material_handles[1] = m_material_pool->Create(m_texture_pool.get());

		wr::Material* red_black_pattern_internal = m_material_pool->GetMaterial(m_material_handles[1]);

		red_black_pattern_internal->SetTexture(wr::TextureType::ALBEDO, red_black_albedo);
		red_black_pattern_internal->SetTexture(wr::TextureType::NORMAL, red_black_normal);
		red_black_pattern_internal->SetTexture(wr::TextureType::ROUGHNESS, red_black_roughness);
		red_black_pattern_internal->SetTexture(wr::TextureType::METALLIC, red_black_metallic);

		// Create Material
		m_material_handles[2] = m_material_pool->Create(m_texture_pool.get());

		wr::Material* metal_material_internal = m_material_pool->GetMaterial(m_material_handles[2]);

		metal_material_internal->SetTexture(wr::TextureType::ALBEDO, metal_albedo);
		metal_material_internal->SetTexture(wr::TextureType::NORMAL, metal_normal);
		metal_material_internal->SetTexture(wr::TextureType::ROUGHNESS, metal_roughness);
		metal_material_internal->SetTexture(wr::TextureType::METALLIC, metal_metallic);

		m_material_handles[3] = m_material_pool->Create(m_texture_pool.get());

		wr::Material* brick_tiles_mat_internal = m_material_pool->GetMaterial(m_material_handles[3]);

		brick_tiles_mat_internal->SetTexture(wr::TextureType::ALBEDO, brick_tiles_albedo);
		brick_tiles_mat_internal->SetTexture(wr::TextureType::NORMAL, brick_tiles_normal);
		brick_tiles_mat_internal->SetTexture(wr::TextureType::ROUGHNESS, brick_tiles_roughness);
		brick_tiles_mat_internal->SetTexture(wr::TextureType::METALLIC, brick_tiles_metallic);

		m_material_handles[4] = m_material_pool->Create(m_texture_pool.get());

		wr::Material* leather_material_internal = m_material_pool->GetMaterial(m_material_handles[4]);

		leather_material_internal->SetTexture(wr::TextureType::ALBEDO, leather_albedo);
		leather_material_internal->SetTexture(wr::TextureType::NORMAL, leather_normal);
		leather_material_internal->SetTexture(wr::TextureType::ROUGHNESS, leather_roughness);
		leather_material_internal->SetTexture(wr::TextureType::METALLIC, leather_metallic);

		m_material_handles[5] = m_material_pool->Create(m_texture_pool.get());

		wr::Material* blue_tiles_material_internal = m_material_pool->GetMaterial(m_material_handles[5]);

		blue_tiles_material_internal->SetTexture(wr::TextureType::ALBEDO, blue_tiles_albedo);
		blue_tiles_material_internal->SetTexture(wr::TextureType::NORMAL, blue_tiles_normal);
		blue_tiles_material_internal->SetTexture(wr::TextureType::ROUGHNESS, blue_tiles_roughness);
		blue_tiles_material_internal->SetTexture(wr::TextureType::METALLIC, blue_tiles_metallic);

		m_material_handles[6] = m_material_pool->Create(m_texture_pool.get());

		wr::Material* gold_material_internal = m_material_pool->GetMaterial(m_material_handles[6]);

		gold_material_internal->SetTexture(wr::TextureType::ALBEDO, gold_albedo);
		gold_material_internal->SetTexture(wr::TextureType::NORMAL, gold_normal);
		gold_material_internal->SetTexture(wr::TextureType::ROUGHNESS, gold_roughness);
		gold_material_internal->SetTexture(wr::TextureType::METALLIC, gold_metallic);

		m_material_handles[7] = m_material_pool->Create(m_texture_pool.get());

		wr::Material* marble_material_internal = m_material_pool->GetMaterial(m_material_handles[7]);

		marble_material_internal->SetTexture(wr::TextureType::ALBEDO, marble_albedo);
		marble_material_internal->SetTexture(wr::TextureType::NORMAL, marble_normal);
		marble_material_internal->SetTexture(wr::TextureType::ROUGHNESS, marble_roughness);
		marble_material_internal->SetTexture(wr::TextureType::METALLIC, marble_metallic);

		m_material_handles[8] = m_material_pool->Create(m_texture_pool.get());

		wr::Material* floreal_tiles_internal = m_material_pool->GetMaterial(m_material_handles[8]);

		floreal_tiles_internal->SetTexture(wr::TextureType::ALBEDO, floreal_tiles_albedo);
		floreal_tiles_internal->SetTexture(wr::TextureType::NORMAL, floreal_tiles_normal);
		floreal_tiles_internal->SetTexture(wr::TextureType::ROUGHNESS, floreal_tiles_roughness);
		floreal_tiles_internal->SetTexture(wr::TextureType::METALLIC, floreal_tiles_metallic);

		m_material_handles[9] = m_material_pool->Create(m_texture_pool.get());

		wr::Material* bw_tiles_internal = m_material_pool->GetMaterial(m_material_handles[9]);

		bw_tiles_internal->SetTexture(wr::TextureType::ALBEDO, bw_tiles_albedo);
		bw_tiles_internal->SetTexture(wr::TextureType::NORMAL, bw_tiles_normal);
		bw_tiles_internal->SetTexture(wr::TextureType::ROUGHNESS, bw_tiles_roughness);
		bw_tiles_internal->SetTexture(wr::TextureType::METALLIC, bw_tiles_metallic);
		bw_tiles_internal->SetTexture(wr::TextureType::EMISSIVE, bw_tiles_emissive);
	}

	{
		wr::MeshData<wr::VertexColor> mesh;

		mesh.m_indices = {
			2, 1, 0, 3, 2, 0
		};

		mesh.m_vertices = {
			//POS                UV            NORMAL                TANGENT            BINORMAL	COLOR
			{  1,  1,  0,        1, 1,        0, 0, -1,            0, 0, 1,        0, 1, 0,			0, 0, 0 },
			{  1, -1,  0,        1, 0,        0, 0, -1,            0, 0, 1,        0, 1, 0,			0, 0, 0 },
			{ -1, -1,  0,        0, 0,        0, 0, -1,            0, 0, 1,        0, 1, 0,			0, 0, 0 },
			{ -1,  1,  0,        0, 1,        0, 0, -1,            0, 0, 1,        0, 1, 0,			0, 0, 0 },
		};

		m_plane_model = m_model_pool->Load<wr::VertexColor>(m_material_pool.get(), m_texture_pool.get(), "resources/models/plane.fbx");

		for (auto& m : m_plane_model->m_meshes)
		{
			m.second = m_material_handles[0];
		}
	}

	{
		{
			m_cube_model = m_model_pool->Load<wr::VertexColor>(m_material_pool.get(), m_texture_pool.get(), "resources/models/cube.fbx");
			for (auto& m : m_cube_model->m_meshes)
			{
				m.second = m_material_handles[0];
			}

		}

		{
			m_material_knob_model = m_model_pool->Load<wr::VertexColor>(m_material_pool.get(), m_texture_pool.get(), "resources/models/material_ball.fbx");
			for (auto& m : m_material_knob_model->m_meshes)
			{
				m.second = m_material_handles[0];
			}
		}
	}
}

void EmiblScene::BuildScene(unsigned int width, unsigned int height, void* extra)
{
	m_camera = m_scene_graph->CreateChild<DebugCamera>(nullptr, 90.f, (float)width / (float)height);
	m_camera->SetPosition({ 500.0f, 60.0f, 260.0f });
	m_camera->SetRotation({ -16._deg, 0._deg, 0._deg });
	m_camera->SetSpeed(100.0f);

	auto skybox = m_scene_graph->CreateChild<wr::SkyboxNode>(nullptr, m_skybox);

	// Geometry
	for (size_t i = 0; i < 10; ++i)
	{
		std::vector<wr::MaterialHandle> mat_handles(m_material_knob_model->m_meshes.size(), m_material_handles[i]);

		m_models[i] = m_scene_graph->CreateChild<wr::MeshNode>(nullptr, m_material_knob_model);
		m_models[i]->SetMaterials(mat_handles);
		m_platforms[i] = m_scene_graph->CreateChild<wr::MeshNode>(nullptr, m_cube_model);
		m_platforms[i]->SetMaterials(mat_handles);
		m_platforms[i]->SetScale({ 38, 1, 38 });
	}

	m_models[9]->SetPosition({ -500,	0	,	-160 });
	m_platforms[9]->SetPosition({ -500, -3, -160 });

	m_models[8]->SetPosition({ -250,	0	,	-160 });
	m_platforms[8]->SetPosition({ -250,	-3	,	-160 });

	m_models[7]->SetPosition({ 0,		0	,	-160 });
	m_platforms[7]->SetPosition({ 0,		-3	,	-160 });

	m_models[6]->SetPosition({ +250,		0	,-160 });
	m_platforms[6]->SetPosition({ +250,		-3	,-160 });

	m_models[5]->SetPosition({ +500,		0	,	-160 });
	m_platforms[5]->SetPosition({ +500,		-3	,	-160 });

	m_models[4]->SetPosition({ -500,0	,	160 });
	m_platforms[4]->SetPosition({ -500, -3 ,	160 });

	m_models[3]->SetPosition({ -250,		0	,	160 });
	m_platforms[3]->SetPosition({ -250,		-3	,	160 });

	m_models[2]->SetPosition({ 0,				0	,160 });
	m_platforms[2]->SetPosition({ 0,				-3	,160 });

	m_models[1]->SetPosition({ +250,0	,	160 });
	m_platforms[1]->SetPosition({ +250, -3	,	160 });

	m_models[0]->SetPosition({ +500,		0	,	160 });
	m_platforms[0]->SetPosition({ +500,		-3	,	160 });

	auto dir_light = m_scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::DIRECTIONAL, DirectX::XMVECTOR{ 0, 1, 0 });
	dir_light->SetDirectional({ 360_deg - 136_deg, 0, 0 }, { 4, 4, 4 });
}

void EmiblScene::Update()
{
	m_camera->Update(ImGui::GetIO().DeltaTime);
}

#pragma once

#include "wisp.hpp"
#include "window.hpp"
#include "scene_graph/scene_graph.hpp"
#include "imgui/imgui.hpp"
#include "debug_camera.hpp"

namespace emibl_scene
{
  namespace resources
  {
    //////////////////////////
    // RESOURCES
    //////////////////////////

    static std::shared_ptr<wr::ModelPool> model_pool;
    static std::shared_ptr<wr::TexturePool> texture_pool;
    static std::shared_ptr<wr::MaterialPool> material_pool;

    static wr::Model* cube_model;
    static wr::Model* plane_model;
    static wr::Model* test_model;
    static wr::Model* material_knob;

    static wr::MaterialHandle rusty_metal_material;

    static wr::MaterialHandle material_handles[10];

    static wr::TextureHandle equirectangular_environment_map;

    static wr::TextureHandle flat_normal;

    void CreateResources(wr::RenderSystem* render_system)
    {
      texture_pool = render_system->CreateTexturePool();
      material_pool = render_system->CreateMaterialPool(256);

      // Load Texture.
      wr::TextureHandle white = texture_pool->LoadFromFile("resources/materials/white.png", false, true);
      wr::TextureHandle black = texture_pool->LoadFromFile("resources/materials/black.png", false, true);
      flat_normal = texture_pool->LoadFromFile("resources/materials/flat_normal.png", false, true);

      wr::TextureHandle metal_splotchy_albedo = texture_pool->LoadFromFile("resources/materials/metal-splotchy-albedo.png", true, true);
      wr::TextureHandle metal_splotchy_normal = texture_pool->LoadFromFile("resources/materials/metal-splotchy-normal-dx.png", false, true);
      wr::TextureHandle metal_splotchy_roughness = texture_pool->LoadFromFile("resources/materials/metal-splotchy-rough.png", false, true);
      wr::TextureHandle metal_splotchy_metallic = texture_pool->LoadFromFile("resources/materials/metal-splotchy-metal.png", false, true);

      wr::TextureHandle mahogfloor_albedo = texture_pool->LoadFromFile("resources/materials/textures/Cerberus_A.png", true, true);
      wr::TextureHandle mahogfloor_normal = texture_pool->LoadFromFile("resources/materials/textures/Cerberus_N.png", false, true);
      wr::TextureHandle mahogfloor_roughness = texture_pool->LoadFromFile("resources/materials/textures/Cerberus_R.png", false, true);
      wr::TextureHandle mahogfloor_metallic = texture_pool->LoadFromFile("resources/materials/textures/Cerberus_M.png", false, true);

      wr::TextureHandle red_black_albedo = texture_pool->LoadFromFile("resources/materials/textures/Gilza_baseColor.jpeg", true, true);
      wr::TextureHandle red_black_normal = texture_pool->LoadFromFile("resources/materials/textures/Gilza_normal.png", false, true);
      wr::TextureHandle red_black_roughness = texture_pool->LoadFromFile("resources/materials/textures/Gilza_roughness.png", false, true);
      wr::TextureHandle red_black_metallic = texture_pool->LoadFromFile("resources/materials/textures/Gilza_metallic.png", false, true);

      wr::TextureHandle metal_albedo = texture_pool->LoadFromFile("resources/materials/textures/Pula_baseColor.jpeg", true, true);
      wr::TextureHandle metal_normal = texture_pool->LoadFromFile("resources/materials/textures/Pula_normal.png", false, true);
      wr::TextureHandle metal_roughness = texture_pool->LoadFromFile("resources/materials/textures/Pula_roughness.png", false, true);
      wr::TextureHandle metal_metallic = texture_pool->LoadFromFile("resources/materials/textures/Pula_metallic.png", false, true);

      wr::TextureHandle brick_tiles_albedo = texture_pool->LoadFromFile("resources/materials/brick_tiles/albedo.png", true, true);
      wr::TextureHandle brick_tiles_normal = texture_pool->LoadFromFile("resources/materials/brick_tiles/normal.png", false, true);
      wr::TextureHandle brick_tiles_roughness = texture_pool->LoadFromFile("resources/materials/brick_tiles/roughness.png", false, true);
      wr::TextureHandle brick_tiles_metallic = texture_pool->LoadFromFile("resources/materials/brick_tiles/metallic.png", false, true);

      wr::TextureHandle leather_albedo = texture_pool->LoadFromFile("resources/materials/leather_with_metal/albedo.png", true, true);
      wr::TextureHandle leather_normal = texture_pool->LoadFromFile("resources/materials/leather_with_metal/normal.png", false, true);
      wr::TextureHandle leather_roughness = texture_pool->LoadFromFile("resources/materials/leather_with_metal/roughness.png", false, true);
      wr::TextureHandle leather_metallic = texture_pool->LoadFromFile("resources/materials/leather_with_metal/metallic.png", false, true);

      wr::TextureHandle blue_tiles_albedo = texture_pool->LoadFromFile("resources/materials/blue_tiles/albedo.png", true, true);
      wr::TextureHandle blue_tiles_normal = texture_pool->LoadFromFile("resources/materials/blue_tiles/normal.png", false, true);
      wr::TextureHandle blue_tiles_roughness = texture_pool->LoadFromFile("resources/materials/blue_tiles/roughness.png", false, true);
      wr::TextureHandle blue_tiles_metallic = texture_pool->LoadFromFile("resources/materials/blue_tiles/metallic.png", false, true);

      wr::TextureHandle gold_albedo = texture_pool->LoadFromFile("resources/materials/gold_scuffed/albedo.png", true, true);
      wr::TextureHandle gold_normal = texture_pool->LoadFromFile("resources/materials/gold_scuffed/normal.png", false, true);
      wr::TextureHandle gold_roughness = texture_pool->LoadFromFile("resources/materials/gold_scuffed/roughness.png", false, true);
      wr::TextureHandle gold_metallic = texture_pool->LoadFromFile("resources/materials/gold_scuffed/metallic.png", false, true);

      wr::TextureHandle marble_albedo = texture_pool->LoadFromFile("resources/materials/marble_speckled/albedo.png", true, true);
      wr::TextureHandle marble_normal = texture_pool->LoadFromFile("resources/materials/marble_speckled/normal.png", false, true);
      wr::TextureHandle marble_roughness = texture_pool->LoadFromFile("resources/materials/marble_speckled/roughness.png", false, true);
      wr::TextureHandle marble_metallic = texture_pool->LoadFromFile("resources/materials/marble_speckled/metallic.png", false, true);


      wr::TextureHandle floreal_tiles_albedo = texture_pool->LoadFromFile("resources/materials/floreal_tiles/albedo.png", true, true);
      wr::TextureHandle floreal_tiles_normal = texture_pool->LoadFromFile("resources/materials/floreal_tiles/normal.png", false, true);
      wr::TextureHandle floreal_tiles_roughness = texture_pool->LoadFromFile("resources/materials/floreal_tiles/roughness.png", false, true);
      wr::TextureHandle floreal_tiles_metallic = texture_pool->LoadFromFile("resources/materials/floreal_tiles/metallic.png", false, true);

      wr::TextureHandle bw_tiles_albedo = texture_pool->LoadFromFile("resources/materials/bw_tiles_gold_lining/albedo.png", true, true);
      wr::TextureHandle bw_tiles_normal = texture_pool->LoadFromFile("resources/materials/bw_tiles_gold_lining/normal.png", false, true);
      wr::TextureHandle bw_tiles_roughness = texture_pool->LoadFromFile("resources/materials/bw_tiles_gold_lining/roughness.png", false, true);
      wr::TextureHandle bw_tiles_metallic = texture_pool->LoadFromFile("resources/materials/bw_tiles_gold_lining/metallic.png", false, true);

      equirectangular_environment_map = texture_pool->LoadFromFile("resources/materials/Arches_E_PineTree_3k.hdr", false, false);

      {
        // Create Material
        rusty_metal_material = material_pool->Create();

        wr::Material* rusty_metal_internal = material_pool->GetMaterial(rusty_metal_material);

        rusty_metal_internal->SetAlbedo(metal_splotchy_albedo);
        rusty_metal_internal->SetNormal(metal_splotchy_normal);
        rusty_metal_internal->SetRoughness(metal_splotchy_roughness);
        rusty_metal_internal->SetMetallic(metal_splotchy_metallic);

        // Create Material
        material_handles[0] = material_pool->Create();

        wr::Material* mahogfloor_material_internal = material_pool->GetMaterial(material_handles[0]);

        mahogfloor_material_internal->SetAlbedo(mahogfloor_albedo);
        mahogfloor_material_internal->SetNormal(mahogfloor_normal);
        mahogfloor_material_internal->SetRoughness(mahogfloor_roughness);
        mahogfloor_material_internal->SetMetallic(mahogfloor_metallic);

        // Create Material
        material_handles[1] = material_pool->Create();

        wr::Material* red_black_pattern_internal = material_pool->GetMaterial(material_handles[1]);

        red_black_pattern_internal->SetAlbedo(red_black_albedo);
        red_black_pattern_internal->SetNormal(red_black_normal);
        red_black_pattern_internal->SetRoughness(red_black_roughness);
        red_black_pattern_internal->SetMetallic(red_black_metallic);

        // Create Material
        material_handles[2] = material_pool->Create();

        wr::Material* metal_material_internal = material_pool->GetMaterial(material_handles[2]);

        metal_material_internal->SetAlbedo(metal_albedo);
        metal_material_internal->SetNormal(metal_normal);
        metal_material_internal->SetRoughness(metal_roughness);
        metal_material_internal->SetMetallic(metal_metallic);

        material_handles[3] = material_pool->Create();

        wr::Material* brick_tiles_mat_internal = material_pool->GetMaterial(material_handles[3]);

        brick_tiles_mat_internal->SetAlbedo(brick_tiles_albedo);
        brick_tiles_mat_internal->SetNormal(brick_tiles_normal);
        brick_tiles_mat_internal->SetRoughness(brick_tiles_roughness);
        brick_tiles_mat_internal->SetMetallic(brick_tiles_metallic);

        material_handles[4] = material_pool->Create();

        wr::Material* leather_material_internal = material_pool->GetMaterial(material_handles[4]);

        leather_material_internal->SetAlbedo(leather_albedo);
        leather_material_internal->SetNormal(leather_normal);
        leather_material_internal->SetRoughness(leather_roughness);
        leather_material_internal->SetMetallic(leather_metallic);

        material_handles[5] = material_pool->Create();

        wr::Material* blue_tiles_material_internal = material_pool->GetMaterial(material_handles[5]);

        blue_tiles_material_internal->SetAlbedo(blue_tiles_albedo);
        blue_tiles_material_internal->SetNormal(blue_tiles_normal);
        blue_tiles_material_internal->SetRoughness(blue_tiles_roughness);
        blue_tiles_material_internal->SetMetallic(blue_tiles_metallic);

        material_handles[6] = material_pool->Create();

        wr::Material* gold_material_internal = material_pool->GetMaterial(material_handles[6]);

        gold_material_internal->SetAlbedo(gold_albedo);
        gold_material_internal->SetNormal(gold_normal);
        gold_material_internal->SetRoughness(gold_roughness);
        gold_material_internal->SetMetallic(gold_metallic);

        material_handles[7] = material_pool->Create();

        wr::Material* marble_material_internal = material_pool->GetMaterial(material_handles[7]);

        marble_material_internal->SetAlbedo(marble_albedo);
        marble_material_internal->SetNormal(marble_normal);
        marble_material_internal->SetRoughness(marble_roughness);
        marble_material_internal->SetMetallic(marble_metallic);

        material_handles[8] = material_pool->Create();

        wr::Material* floreal_tiles_internal = material_pool->GetMaterial(material_handles[8]);

        floreal_tiles_internal->SetAlbedo(floreal_tiles_albedo);
        floreal_tiles_internal->SetNormal(floreal_tiles_normal);
        floreal_tiles_internal->SetRoughness(floreal_tiles_roughness);
        floreal_tiles_internal->SetMetallic(floreal_tiles_metallic);

        material_handles[9] = material_pool->Create();

        wr::Material* bw_tiles_internal = material_pool->GetMaterial(material_handles[9]);

        bw_tiles_internal->SetAlbedo(bw_tiles_albedo);
        bw_tiles_internal->SetNormal(bw_tiles_normal);
        bw_tiles_internal->SetRoughness(bw_tiles_roughness);
        bw_tiles_internal->SetMetallic(bw_tiles_metallic);
      }

      model_pool = render_system->CreateModelPool(64_mb, 64_mb);

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

        plane_model = model_pool->Load<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/plane.fbx");

        for (auto& m : plane_model->m_meshes)
        {
          m.second = material_handles[0];
        }
      }

      {
        {
          test_model = model_pool->Load<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/materials/Bullet.fbx");

          test_model->m_meshes[0].second = material_handles[1];
          test_model->m_meshes[1].second = material_handles[2];

          cube_model = model_pool->Load<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/models/cube.fbx");
          for (auto& m : cube_model->m_meshes)
          {
            m.second = material_handles[0];
          }

        }

        {
          material_knob = model_pool->Load<wr::VertexColor>(material_pool.get(), texture_pool.get(), "resources/materials/Cerberus_LP.fbx");
          for (auto& m : material_knob->m_meshes)
          {
            m.second = material_handles[0];
          }
        }
      }
    }

    void ReleaseResources()
    {
      model_pool.reset();
      texture_pool.reset();
      material_pool.reset();
    };

  }


  static std::shared_ptr<DebugCamera> camera;
  static std::shared_ptr<wr::Node> root_node;

  static std::shared_ptr<wr::LightNode> directional_light;
  static std::shared_ptr<wr::LightNode> light_node_1;
  static std::shared_ptr<wr::LightNode> light_node_2;
  static std::shared_ptr<wr::LightNode> light_node_3;
  static std::shared_ptr<wr::MeshNode> test_model;
  static std::shared_ptr<wr::MeshNode> cube_node;
  static float t = 0;
  static float lerp_t = 0.0f;

  std::shared_ptr<wr::MeshNode> models[10];
  std::shared_ptr<wr::MeshNode> platforms[10];

  static DirectX::XMVECTOR camera_start_pos = { 500.0f, 60.0f, 260.0f };
  static DirectX::XMVECTOR camera_end_pos = { -500.0f, 60.0f, 260.0f };

  static DirectX::XMVECTOR bullets_start_pos[3] =
  {
    {0.0f, 28.0f, 143.0f, 1.0f}, {-4.15f, 21.5f, 143.0f, 1.0f}, {4.15f, 21.5f, 143.0f, 1.0f}
  };

  static DirectX::XMVECTOR bullets_end_pos[3] =
  {
    {0.0f, 28.0f, 678.659f, 1.0f}, {-4.15f, 21.5f, 704.862f, 1.0f}, {4.15f, 21.5f, 857.585f, 1.0f}
  };

  static DirectX::XMVECTOR bullets_current_pos[3] =
  {
    bullets_start_pos[0], bullets_start_pos[1], bullets_start_pos[2]
  };

  void CreateScene(wr::SceneGraph* scene_graph, wr::Window* window)
  {
    root_node = scene_graph->CreateChild<wr::Node>();

    camera = scene_graph->CreateChild<DebugCamera>(nullptr, 90.f, (float)window->GetWidth() / (float)window->GetHeight());
    camera->SetPosition({ 100.0f, 66.666f, 180.7f });
    camera->SetRotation({ -1.267_deg, 50.0_deg, 0._deg });
    camera->SetSpeed(100.0f);

    scene_graph->m_skybox = resources::equirectangular_environment_map;
    auto skybox = scene_graph->CreateChild<wr::SkyboxNode>(nullptr, resources::equirectangular_environment_map);

    // Geometry
    for (size_t i = 0; i < 1; ++i)
    {
      std::vector<wr::MaterialHandle> mat_handles(resources::material_knob->m_meshes.size(), resources::material_handles[i]);

      models[i] = scene_graph->CreateChild<wr::MeshNode>(root_node, resources::material_knob);
      models[i]->SetMaterials(mat_handles);
      models[i]->SetPosition({ 0.0f, 20.5f, 120.0f });
      models[i]->SetRotation({ 90.0_deg, 0.0f, 0.0f });
      /*platforms[i] = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::cube_model);
      platforms[i]->SetMaterials(mat_handles);
      platforms[i]->SetScale({ 38, 1, 38 });*/
    }

    std::vector<wr::MaterialHandle> mat_handles_2;
    mat_handles_2.push_back(resources::material_handles[1]);
    mat_handles_2.push_back(resources::material_handles[2]);


    models[1] = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::test_model);
    models[1]->SetMaterials(mat_handles_2);
    models[1]->SetPosition(bullets_start_pos[0]);
    models[1]->SetRotation({ 0.0f, 0.0f, 0.0f });
    models[1]->SetScale({ 5.0f, 5.0f, 5.0f });

    models[2] = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::test_model);
    models[2]->SetMaterials(mat_handles_2);
    models[2]->SetPosition(bullets_start_pos[1]);
    models[2]->SetRotation({ 0.0f, 0.0f, 0.0f });
    models[2]->SetScale({ 5.0f, 5.0f, 5.0f });

    models[3] = scene_graph->CreateChild<wr::MeshNode>(nullptr, resources::test_model);
    models[3]->SetMaterials(mat_handles_2);
    models[3]->SetPosition(bullets_start_pos[2]);
    models[3]->SetRotation({ 0.0f, 0.0f, 0.0f });
    models[3]->SetScale({ 5.0f, 5.0f, 5.0f });

    /*models[9]->SetPosition({ -500,	0	,	-160 });
    platforms[9]->SetPosition({ -500, -3, -160 });

    models[8]->SetPosition({ -250,	0	,	-160 });
    platforms[8]->SetPosition({ -250,	-3	,	-160 });

    models[7]->SetPosition({ 0,		0	,	-160 });
    platforms[7]->SetPosition({ 0,		-3	,	-160 });

    models[6]->SetPosition({ +250,		0	,-160 });
    platforms[6]->SetPosition({ +250,		-3	,-160 });

    models[5]->SetPosition({ +500,		0	,	-160 });
    platforms[5]->SetPosition({ +500,		-3	,	-160 });

    models[4]->SetPosition({ -500,0	,	160 });
    platforms[4]->SetPosition({ -500, -3 ,	160 });

    models[3]->SetPosition({ -250,		0	,	160 });
    platforms[3]->SetPosition({ -250,		-3	,	160 });

    models[2]->SetPosition({ 0,				0	,160 });
    platforms[2]->SetPosition({ 0,				-3	,160 });

    models[1]->SetPosition({ +250,0	,	160 });
    platforms[1]->SetPosition({ +250, -3	,	160 });

    models[0]->SetPosition({ +500,		0	,	160 });
    platforms[0]->SetPosition({ +500,		-3	,	160 });*/

    for (uint32_t i = 0; i < 10; ++i)
    {
      //models[i]->SetScale({ 0.5f, 0.5f, 0.5f });
      //models[i]->SetRotation({ 90_deg, 0, 0 });
    }

    directional_light = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::DIRECTIONAL, DirectX::XMVECTOR{ 1000, 1000, 1000 });
    directional_light->SetDirectional({ -45.0_deg, 0.0f, 0.0f }, { 1000, 1000, 1000 });

    light_node_1 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 25555, 1640, 0 });
    light_node_1->SetPoint({ 4.174, 22.448, 150.316 }, 54, { 25555, 1640, 0 });

    light_node_2 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 25555, 1640, 0 });
    light_node_2->SetPoint({ -2.916, 22.209f, 150.316f }, 54, { 25555, 1640, 0 });

    light_node_3 = scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 25555, 1640, 0 });
    light_node_3->SetPoint({ 0.0f, 29.502f, 150.136f }, 54, { 25555, 1640, 0 });
  }

  static bool restart_scene = false;
  static bool paused = false;
  static bool camera_follow_bullet = false;

  void UpdateScene()
  {
    float deltaTime = ImGui::GetIO().DeltaTime;

    if (!paused)
    {
      static float waiting = 0.0f;
      static bool start_lerp = false;

      static float begin_rot = 0.0f;
      static float end_rot = -45.0f;
      static float curr_rot = begin_rot;
      static float light_radius_1 = 0.0f;
      static float light_radius_2 = 0.0f;
      static float light_radius_3 = 0.0f;
      static float light_t = 0.0f;
      static float light_lerp_begin = 0.00f;
      static float light_lerp_end = 0.15f;

      auto lerp = [](float v0, float v1, float t)->float
      {
        return (1 - t) * v0 + t * v1;
      };

      if (restart_scene)
      {
        t = 0.0f;
        light_t = 0.0f;

        restart_scene = false;
      }

      t += deltaTime * 0.01f;

      if (t > light_lerp_begin && t < light_lerp_end * 0.5f)
      {
        light_t += deltaTime * 2.0f;

        light_radius_1 = lerp(0, 3, light_t);
        light_radius_2 = lerp(0, 2, light_t);
        light_radius_3 = lerp(0, 4, light_t);

        light_node_1->SetPoint({ 4.174, 22.448, 150.316 }, light_radius_1, { 2555, 160, 0 });
        light_node_2->SetPoint({ -2.916, 22.209f, 150.316f }, light_radius_2, { 2555, 160, 0 });
        light_node_3->SetPoint({ 0.0f, 42.502f, 150.136f }, light_radius_3, { 2555, 160, 0 });
      }
      else if (t >= light_lerp_end * 0.5f && t < light_lerp_end)
      {
        light_t -= deltaTime * 20.0f;

        if (light_t < 0.0f)
        {
          light_t = 0.0f;
        }

        light_radius_1 = lerp(0, 3, light_t);
        light_radius_2 = lerp(0, 2, light_t);
        light_radius_3 = lerp(0, 4, light_t);

        light_node_1->SetPoint({ 4.174, 22.448, 150.316 }, light_radius_1, { 2555, 160, 0 });
        light_node_2->SetPoint({ -2.916, 22.209f, 150.316f }, light_radius_2, { 2555, 160, 0 });
        light_node_3->SetPoint({ 0.0f, 42.502f, 150.136f }, light_radius_3, { 2555, 160, 0 });
      }

      if (t >= 1.0f)
      {
        t = 1.0f;
      }

      //if (lerp_t < 1.0f && start_lerp)
      //{
      //	lerp_t += deltaTime * 0.05f;

      //	if (lerp_t > 1.0f)
      //	{
      //		lerp_t = 1.0f;
      //	}

      //	DirectX::XMVECTOR new_cam_pos = DirectX::XMVectorLerp(camera_start_pos, camera_end_pos, lerp_t);
      //	camera->SetPosition(new_cam_pos);
      //}


      curr_rot = lerp(begin_rot, end_rot, t);

      for (uint32_t i = 0; i < 10; ++i)
      {
        //models[i]->SetRotation({ 0, DirectX::XMConvertToRadians(t), 0 });
        root_node->SetRotation({ DirectX::XMConvertToRadians(curr_rot), 0, 0 });
      }

      bullets_current_pos[0] = DirectX::XMVectorLerp(bullets_start_pos[0], bullets_end_pos[0], t);
      bullets_current_pos[1] = DirectX::XMVectorLerp(bullets_start_pos[1], bullets_end_pos[1], t);
      bullets_current_pos[2] = DirectX::XMVectorLerp(bullets_start_pos[2], bullets_end_pos[2], t);

      models[1]->SetPosition(bullets_current_pos[0]);
      models[2]->SetPosition(bullets_current_pos[1]);
      models[3]->SetPosition(bullets_current_pos[2]);

      models[1]->SetRotation({ 0, 0, DirectX::XMConvertToRadians(t * 800) });
      models[2]->SetRotation({ 0, 0, DirectX::XMConvertToRadians(t * 1400) });
      models[3]->SetRotation({ 0, 0, DirectX::XMConvertToRadians(t * 2000) });

      if (camera_follow_bullet)
      {
        DirectX::XMVECTOR camera_rot = { DirectX::XMConvertToRadians(14.6f), DirectX::XMConvertToRadians(54.2f), 0.0f };

        camera->SetPosition({ 14.0f, 21.0f, DirectX::XMVectorGetZ(bullets_current_pos[1]) + 10.0f });
        camera->SetRotation(camera_rot);
      }
    }

    camera->Update(deltaTime);
  }
} /* cube_scene */
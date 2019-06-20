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
#include "scene.hpp"

#include <DirectXMath.h>
#include <fstream>

#include "json.hpp"

Scene::Scene(std::size_t material_pool_size,
			 std::size_t model_pool_vb_size,
			 std::size_t model_pool_ib_size) :
	m_material_pool_size(material_pool_size),
	m_model_pool_vb_size(model_pool_vb_size),
	m_model_pool_ib_size(model_pool_ib_size)
{

}

Scene::~Scene()
{

}

void Scene::Init(wr::D3D12RenderSystem* rs, unsigned int width, unsigned int height, void* extra)
{
	m_texture_pool = rs->CreateTexturePool();
	m_material_pool = rs->CreateMaterialPool(m_material_pool_size);
	m_model_pool = rs->CreateModelPool(m_model_pool_vb_size, m_model_pool_ib_size);

	LoadResources();

	m_scene_graph = std::make_shared<wr::SceneGraph>(rs);
	BuildScene(width, height, extra);
	rs->InitSceneGraph(*m_scene_graph.get());
}

std::shared_ptr<wr::SceneGraph> Scene::GetSceneGraph()
{
	return m_scene_graph;
}

void Scene::LoadLightsFromJSON()
{
	if (!m_lights_path.has_value())
	{
		LOGW("Tried to load lights from json without a path specified.");
		return;
	}

	// Read JSON file.
	std::ifstream f(m_lights_path.value());
	nlohmann::json json;
	f >> json;

	// Loop over lights
	auto j_lights = json["lights"];

	LOG("Number of lights: {}", j_lights.size() );

	for (std::size_t i = 0; i < j_lights.size(); i++)
	{
		auto j_light = j_lights[i];


		auto type = j_light["type"].get<int>();
		auto color = j_light["color"].get<std::vector<float>>();
		auto pos = j_light["pos"].get<std::vector<float>>();
		auto rot = j_light["rot"].get<std::vector<float>>();
		auto size = j_light["size"].get<int>();
		auto radius = j_light["radius"].get<int>();
		auto angle = j_light["angle"].get<int>();

		auto light = m_scene_graph->CreateChild<wr::LightNode>(nullptr, (wr::LightType)type);
		light->SetColor({ color[0], color[1], color[2] });
		light->SetPosition({ pos[0], pos[1], pos[2] });
		light->SetRotation({ rot[0], rot[1], rot[2] });
		light->SetLightSize(size);
		light->SetRadius(radius);
		light->SetAngle(angle);
	}
}

void Scene::SaveLightsToJSON()
{
	if (!m_lights_path.has_value())
	{
		LOGW("Tried to save lights to json without a path specified.");
		return;
	}

	nlohmann::json j_lights = nlohmann::json::array();

	auto lights = m_scene_graph->GetLightNodes();

	for (auto& light : lights)
	{
		nlohmann::json j_light = nlohmann::json::object();

		j_light["type"] = (int)light->GetType();
		j_light["color"] = { light->m_light->col.x, light->m_light->col.y, light->m_light->col.z };
		j_light["pos"] = light->m_position.m128_f32;
		j_light["rot"] = light->m_rotation_radians.m128_f32;
		j_light["size"] = light->m_light->light_size;
		j_light["radius"] = light->m_light->rad;
		j_light["angle"] = light->m_light->ang;

		j_lights.push_back(j_light);
	}

	// write JSON to  file
	nlohmann::json json;
	json["lights"] = j_lights;

	std::ofstream o(m_lights_path.value());
	o << std::setw(4) << json << std::endl;
}
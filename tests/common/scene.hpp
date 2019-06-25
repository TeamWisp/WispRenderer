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

#include <memory>

#include "wisp.hpp"
#include "d3d12/d3d12_renderer.hpp"
#include "util/user_literals.hpp"

class Scene
{
public:
	Scene(std::size_t material_pool_size,
		  std::size_t model_pool_vb_size,
		  std::size_t model_pool_ib_size);
	virtual ~Scene();

	virtual void Init(wr::D3D12RenderSystem* rs, unsigned int width, unsigned int height, void* extra = nullptr);
	virtual void Update(float delta = 0) = 0;
	std::shared_ptr<wr::SceneGraph> GetSceneGraph();
	template<typename T>
	std::shared_ptr<T> GetCamera();

	void LoadLightsFromJSON();
	void SaveLightsToJSON();
	void LoadMeshesFromJSON(std::vector<DirectX::XMVECTOR>& pos, std::vector<DirectX::XMVECTOR>& rot, std::vector<DirectX::XMVECTOR>& scale);
	void SaveMeshesToJSON();

protected:
	virtual void LoadResources() = 0;
	virtual void BuildScene(unsigned int width, unsigned int height, void* extra = nullptr) = 0;

	const std::size_t m_material_pool_size;
	const std::size_t m_model_pool_vb_size;
	const std::size_t m_model_pool_ib_size;

	std::shared_ptr<wr::SceneGraph> m_scene_graph;
	std::shared_ptr<wr::ModelPool> m_model_pool;
	std::shared_ptr<wr::TexturePool> m_texture_pool;
	std::shared_ptr<wr::MaterialPool> m_material_pool;

	std::optional<std::string> m_lights_path;
	std::optional<std::string> m_models_path;
};

template<typename T>
std::shared_ptr<T> Scene::GetCamera()
{
	return std::static_pointer_cast<T>(m_scene_graph->GetActiveCamera());
}

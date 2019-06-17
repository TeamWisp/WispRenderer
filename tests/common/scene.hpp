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
	virtual void Update(float dt) = 0;
	std::shared_ptr<wr::SceneGraph> GetSceneGraph();
	template<typename T>
	std::shared_ptr<T> GetCamera();

	void LoadLightsFromJSON();
	void SaveLightsToJSON();

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
};

template<typename T>
std::shared_ptr<T> Scene::GetCamera()
{
	return std::static_pointer_cast<T>(m_scene_graph->GetActiveCamera());
}

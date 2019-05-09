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

	virtual void Init(wr::D3D12RenderSystem* rs, unsigned int width, unsigned int height);
	virtual void Update() = 0;
	std::shared_ptr<wr::SceneGraph> GetSceneGraph();

protected:
	virtual void LoadResources() = 0;
	virtual void BuildScene(unsigned int width, unsigned int height) = 0;

	const std::size_t m_material_pool_size;
	const std::size_t m_model_pool_vb_size;
	const std::size_t m_model_pool_ib_size;

	std::shared_ptr<wr::SceneGraph> m_scene_graph;
	std::shared_ptr<wr::ModelPool> m_model_pool;
	std::shared_ptr<wr::TexturePool> m_texture_pool;
	std::shared_ptr<wr::MaterialPool> m_material_pool;
};

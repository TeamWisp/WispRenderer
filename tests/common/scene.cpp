#include "scene.hpp"

Scene::Scene(std::size_t material_pool_size,
			 std::size_t model_pool_vb_size,
			 std::size_t model_pool_ib_size) :
	m_material_pool_size(material_pool_size),
	m_model_pool_vb_size(model_pool_vb_size),
	m_model_pool_ib_size(model_pool_ib_size)
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
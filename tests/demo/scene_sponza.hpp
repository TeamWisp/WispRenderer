
#pragma once

#include "wisp.hpp"
#include "window.hpp"
#include "scene_graph/scene_graph.hpp"
#include "imgui/imgui.hpp"
#include "physics_node.hpp"
#include "debug_camera.hpp"
#include "spline_node.hpp"
#include "../common/scene.hpp"

class SponzaScene : public Scene
{
public:
	SponzaScene();

	void Update(float delta = 0) final;

protected:
	void LoadResources() final;
	void BuildScene(unsigned int width, unsigned int height, void* extra = nullptr) final;

private:
	// Models
	wr::Model* m_sphere_model;
	wr::Model* m_sponza_model;
	wr::ModelData* m_sponza_model_data;

	// Textures
	wr::TextureHandle m_skybox;

	// Materials
	std::vector<wr::MaterialHandle> m_mirror_materials;

	// Nodes
	std::shared_ptr<DebugCamera> m_camera;
	std::shared_ptr<SplineNode> m_camera_spline_node;
	std::shared_ptr<PhysicsMeshNode> m_sponza_node;

	float m_time;
};
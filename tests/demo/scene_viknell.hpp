
#pragma once

#include "wisp.hpp"
#include "window.hpp"
#include "scene_graph/scene_graph.hpp"
#include "imgui/imgui.hpp"
#include "physics_node.hpp"
#include "debug_camera.hpp"
#include "spline_node.hpp"
#include "../common/scene.hpp"

class ViknellScene : public Scene
{
public:
	ViknellScene();

	void Update(float delta = 0) final;

protected:
	void LoadResources() final;
	void BuildScene(unsigned int width, unsigned int height, void* extra = nullptr) final;

private:
	// Models
	wr::Model* m_sphere_model;
	wr::Model* m_plane_model;
	wr::ModelData* m_plane_model_data;
	wr::Model* m_xbot_model;

	// Textures
	wr::TextureHandle m_skybox;

	// Materials
	wr::MaterialHandle m_bamboo_material;
	wr::MaterialHandle m_mirror_material;

	// Nodes
	std::shared_ptr<DebugCamera> m_camera;
	std::shared_ptr<SplineNode> m_camera_spline_node;
	std::shared_ptr<PhysicsMeshNode> m_xbot_node;

	float m_time;
};
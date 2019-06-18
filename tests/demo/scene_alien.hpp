
#pragma once

#include "wisp.hpp"
#include "window.hpp"
#include "scene_graph/scene_graph.hpp"
#include "imgui/imgui.hpp"
#include "physics_node.hpp"
#include "debug_camera.hpp"
#include "spline_node.hpp"
#include "../common/scene.hpp"

class AlienScene : public Scene
{
public:
	AlienScene();

	void Update(float delta = 0) final;

protected:
	void LoadResources() final;
	void BuildScene(unsigned int width, unsigned int height, void* extra = nullptr) final;

private:
	// Models
	wr::Model* m_alien_model;

	// Textures
	wr::TextureHandle m_skybox;

	// Nodes
	std::shared_ptr<DebugCamera> m_camera;
	std::shared_ptr<SplineNode> m_camera_spline_node;
};

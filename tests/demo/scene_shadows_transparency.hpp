
#pragma once

#include "wisp.hpp"
#include "window.hpp"
#include "scene_graph/scene_graph.hpp"
#include "imgui/imgui.hpp"
#include "physics_node.hpp"
#include "debug_camera.hpp"
#include "spline_node.hpp"
#include "../common/scene.hpp"

class TransparencyScene : public Scene
{
public:
	TransparencyScene();

	void Update() final;

protected:
	void LoadResources() final;
	void BuildScene(unsigned int width, unsigned int height, void* extra = nullptr) final;

private:
	// Models
	wr::Model* m_plant_model;

	// Textures
	wr::TextureHandle m_env_map;

	// Materials
	wr::MaterialHandle m_plant_material;
	wr::MaterialHandle m_gray_material;

	// Nodes
	std::shared_ptr<DebugCamera> m_camera;
	std::shared_ptr<wr::MeshNode> m_plant_node;

	float m_time;
};
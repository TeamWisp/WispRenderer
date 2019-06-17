
#pragma once

#include "wisp.hpp"
#include "window.hpp"
#include "scene_graph/scene_graph.hpp"
#include "imgui/imgui.hpp"
#include "debug_camera.hpp"
#include "../common/scene.hpp"

class EmiblScene : public Scene
{
public:
	EmiblScene();

	void Update(float delta = 0) final;

protected:
	void LoadResources() final;
	void BuildScene(unsigned int width, unsigned int height, void* extra = nullptr) final;

private:
	// Models
	wr::Model* m_cube_model;
	wr::Model* m_plane_model;
	wr::Model* m_material_knob_model;

	// Textures
	wr::TextureHandle m_skybox;

	// Materials
	wr::MaterialHandle m_rusty_metal_material;
	wr::MaterialHandle m_knob_material;
	wr::MaterialHandle m_material_handles[10];

	// Nodes
	std::shared_ptr<DebugCamera> m_camera;
	std::shared_ptr<wr::MeshNode> m_models[10];
	std::shared_ptr<wr::MeshNode> m_platforms[10];
};
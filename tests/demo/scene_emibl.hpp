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
	wr::MaterialHandle m_material_handles[11];

	// Nodes
	std::shared_ptr<DebugCamera> m_camera;
	std::shared_ptr<wr::MeshNode> m_models[10];
	std::shared_ptr<wr::MeshNode> m_platforms[10];
};
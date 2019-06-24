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
#include "physics_node.hpp"
#include "debug_camera.hpp"
#include "spline_node.hpp"
#include "../common/scene.hpp"

class SponzaScene : public Scene
{
public:
	SponzaScene();

	void Update(float delta_time) final;

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
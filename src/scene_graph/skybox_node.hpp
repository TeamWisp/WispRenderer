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
#include "scene_graph.hpp"

namespace wr {

	struct SkyboxNode : Node
	{
		explicit SkyboxNode(wr::TextureHandle hdr_texture, std::optional<wr::TextureHandle> cubemap = std::nullopt, std::optional<wr::TextureHandle> irradiance = std::nullopt)
			: Node::Node(typeid(SkyboxNode))
			, m_hdr(hdr_texture) 
			, m_skybox(cubemap)
			, m_irradiance(irradiance)
		{}

		void UpdateSkybox(TextureHandle new_equirectangular, unsigned int frame_idx);

		wr::TextureHandle m_hdr;
		std::optional<wr::TextureHandle> m_skybox;
		std::optional<wr::TextureHandle> m_irradiance;
		std::optional<wr::TextureHandle> m_prefiltered_env_map;
	};

};// namespace wr 
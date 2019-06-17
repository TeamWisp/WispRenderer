// Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "skybox_node.hpp"

namespace wr
{
	void SkyboxNode::UpdateSkybox(TextureHandle new_equirectangular, unsigned int frame_idx)
	{
		m_irradiance.value().m_pool->MarkForUnload(m_irradiance.value(), frame_idx);
		m_irradiance = std::nullopt;

		m_skybox.value().m_pool->MarkForUnload(m_skybox.value(), frame_idx);
		m_skybox = std::nullopt;

		m_prefiltered_env_map.value().m_pool->MarkForUnload(m_prefiltered_env_map.value(), frame_idx);
		m_prefiltered_env_map = std::nullopt;

		if (m_hdr.m_pool)
		{
			m_hdr.m_pool->MarkForUnload(m_hdr, frame_idx);

#ifdef _DEBUG
			//Decide if we want to break when developing in case we enter this function
			//as in theory m_hdr needed to be cleaned from the render task, if that's not the case something went wrong.
			LOGC("[ERROR]: M_HDR is supposed to be an invalid handle at this stage. If that's not the case, the next line of code could leak memory");
#endif // DEBUG
		}

		m_hdr = new_equirectangular;
	}
} /* wr */

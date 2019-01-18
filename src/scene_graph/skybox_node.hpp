#pragma once
#include "scene_graph.hpp"

namespace wr {

	struct SkyboxNode : Node
	{
		SkyboxNode(wr::TextureHandle texture) : m_texture(texture) {}

		wr::TextureHandle m_texture;

	};

};// namespace wr 
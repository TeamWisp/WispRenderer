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

		wr::TextureHandle m_hdr;
		std::optional<wr::TextureHandle> m_skybox;
		std::optional<wr::TextureHandle> m_irradiance;
		std::optional<wr::TextureHandle> m_prefiltered_env_map;
	};

};// namespace wr 
#include "renderer.hpp"

void wr::RenderSystem::RequestRenderTargetSaveToDisc(RenderTarget* render_target, unsigned int index)
{
	m_requested_rt_saves.emplace(std::make_pair(render_target, index));
}

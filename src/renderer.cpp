#include "renderer.hpp"

void wr::RenderSystem::RequestRenderTargetSaveToDisc(std::string const & path, RenderTarget* render_target, unsigned int index)
{
	m_requested_rt_saves.emplace(SaveRenderTargetRequest{ path, render_target, index });
}

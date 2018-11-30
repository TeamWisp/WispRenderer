#include "d3d12_functions.hpp"

namespace wr::d3d12
{

	Viewport CreateViewport(int width, int height)
	{
		Viewport viewport;

		// Define viewport.
		viewport.m_viewport.TopLeftX = 0;
		viewport.m_viewport.TopLeftY = 0;
		viewport.m_viewport.Width = static_cast<float>(width);
		viewport.m_viewport.Height = static_cast<float>(height);
		viewport.m_viewport.MinDepth = 0.0f;
		viewport.m_viewport.MaxDepth = 1.0f;

		// Define scissor rect
		viewport.m_scissor_rect.left = 0;
		viewport.m_scissor_rect.top = 0;
		viewport.m_scissor_rect.right = width;
		viewport.m_scissor_rect.bottom = height;

		return viewport;
	}

	void ResizeViewport(Viewport& viewport, int width, int height)
	{
		// Define viewport.
		viewport.m_viewport.Width = static_cast<float>(width);
		viewport.m_viewport.Height = static_cast<float>(height);

		// Define scissor rect
		viewport.m_scissor_rect.right = width;
		viewport.m_scissor_rect.bottom = height;
	}

} /* wr::d3d12 */
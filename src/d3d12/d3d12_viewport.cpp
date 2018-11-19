#include "d3d12_functions.hpp"

namespace d3d12
{

	Viewport CreateViewport(int width, int height)
	{
		Viewport viewport;

		// Define viewport.
		viewport.m_viewport.TopLeftX = 0;
		viewport.m_viewport.TopLeftY = 0;
		viewport.m_viewport.Width = width;
		viewport.m_viewport.Height = height;
		viewport.m_viewport.MinDepth = 0.0f;
		viewport.m_viewport.MaxDepth = 1.0f;

		// Define scissor rect
		viewport.m_scissor_rect.left = 0;
		viewport.m_scissor_rect.top = 0;
		viewport.m_scissor_rect.right = width;
		viewport.m_scissor_rect.bottom = height;

		return viewport;
	}

} /* d3d12 */
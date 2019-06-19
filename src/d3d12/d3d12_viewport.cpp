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
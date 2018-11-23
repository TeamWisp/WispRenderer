#pragma once

#include <optional>
#include <array>

#include "d3d12/d3d12_enums.hpp"

namespace wr
{

	struct CommandList {};
	struct RenderTarget {};

	struct RenderTargetProperties
	{
		bool m_is_render_window;
		std::optional<unsigned int> width;
		std::optional<unsigned int> height;
		bool m_create_dsv_buffer;
		Format m_dsv_format;
		std::array<Format, 8> m_rtv_formats;
		unsigned int m_num_rtv_formats;

		bool m_clear;
		bool m_clear_depth;
	};

} /* wr */
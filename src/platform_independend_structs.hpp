#pragma once

#include <optional>
#include <array>

#include "d3d12/d3d12_enums.hpp"
#include "util/named_type.hpp"

namespace wr
{
	using CommandList = void;
	using RenderTarget = void;

	struct RenderTargetProperties
	{
		using IsRenderWindow = util::NamedType<bool>;
		using Width = util::NamedType<std::optional<unsigned int>>;
		using Height = util::NamedType<std::optional<unsigned int>>;
		using ExecuteResourceState = util::NamedType<std::optional<ResourceState>>;
		using FinishedResourceState = util::NamedType<std::optional<wr::ResourceState>>;
		using CreateDSVBuffer = util::NamedType<bool>;
		using DSVFormat = util::NamedType<Format>;
		using RTVFormats = util::NamedType<std::array<Format, 8>>;
		using NumRTVFormats = util::NamedType<unsigned int>;
		using Clear = util::NamedType<bool>;
		using ClearDepth = util::NamedType<bool>;
		using ResourceName = util::NamedType<std::wstring>;

		IsRenderWindow m_is_render_window;
		Width m_width;
		Height m_height;
		ExecuteResourceState m_state_execute;
		FinishedResourceState m_state_finished;
		CreateDSVBuffer m_create_dsv_buffer;
		DSVFormat m_dsv_format;
		RTVFormats m_rtv_formats;
		NumRTVFormats m_num_rtv_formats;

		Clear m_clear = Clear(false);
		ClearDepth m_clear_depth = ClearDepth(false);

		ResourceName m_name = ResourceName(std::wstring());
	};

} /* wr */
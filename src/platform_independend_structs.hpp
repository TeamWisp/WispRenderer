#pragma once

#include <optional>
#include <array>
#include <DirectXMath.h>

#include "d3d12/d3d12_enums.hpp"
#include "util/named_type.hpp"
#include "util/user_literals.hpp"

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
		using ResolutionScalar = util::NamedType<float>;

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

		ResolutionScalar m_resolution_scale = ResolutionScalar(1.0f);
	};

	enum class LightType : int
	{
		POINT, DIRECTIONAL, SPOT, FREE
	};

	struct Light
	{
		DirectX::XMFLOAT3 pos = { 0, 0, 0 };			//Position in world space for spot & point
		float rad = 5.f;								//Radius for point, height for spot

		DirectX::XMFLOAT3 col = { 1, 1, 1 };			//Color (and strength)
		uint32_t tid = (uint32_t)LightType::FREE;		//Type id; LightType::x

		DirectX::XMFLOAT3 dir = { 0, 0, 1 };			//Direction for spot & directional
		float ang = 40._deg;							//Angle for spot; in radians

		DirectX::XMFLOAT3 padding;
		float light_size = 0.0f;
	};

} /* wr */
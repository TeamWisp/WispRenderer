#pragma once

#include <d3d12.h>

namespace d3d12::settings
{

	enum DebugLayer
	{
		ENABLE,
		DISABLE,
		ENABLE_WITH_GPU_VALIDATION
	};

	static const constexpr D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_12_1;
	static const constexpr DXGI_FORMAT back_buffer_format = DXGI_FORMAT_R8G8B8A8_UNORM;
	static const constexpr DXGI_SWAP_EFFECT flip_mode = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	static const constexpr DXGI_SWAP_CHAIN_FLAG swapchain_flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	static const constexpr DXGI_SCALING swapchain_scaling = DXGI_SCALING_STRETCH;
	static const constexpr DXGI_ALPHA_MODE swapchain_alpha_mode = DXGI_ALPHA_MODE_UNSPECIFIED;
	static const constexpr bool enable_gpu_timeout = false;
	static const constexpr bool enable_debug_factory = false;
	static const DebugLayer enable_debug_layer = DebugLayer::ENABLE_WITH_GPU_VALIDATION;
	static const constexpr char* default_shader_model = "5.0";
	static const constexpr std::uint8_t num_back_buffers = 3;

} /* d3d12::settings */
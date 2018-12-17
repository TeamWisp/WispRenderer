#pragma once

#include <d3d12.h>
#include <cstdint>
#include <dxgi1_5.h>

namespace wr::d3d12::settings
{

	enum class DebugLayer
	{
		ENABLE,
		DISABLE,
		ENABLE_WITH_GPU_VALIDATION
	};
	
	static const std::vector<D3D_FEATURE_LEVEL> possible_feature_levels =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

	static const constexpr Format back_buffer_format = Format::R8G8B8A8_UNORM;
	static const constexpr DXGI_SWAP_EFFECT flip_mode = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	static const constexpr DXGI_SWAP_CHAIN_FLAG swapchain_flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	static const constexpr  DXGI_SCALING swapchain_scaling = DXGI_SCALING_STRETCH;
	static const constexpr DXGI_ALPHA_MODE swapchain_alpha_mode = DXGI_ALPHA_MODE_UNSPECIFIED;
	static const constexpr bool enable_gpu_timeout = false;
	static const constexpr bool enable_debug_factory = true;
	static const constexpr DebugLayer enable_debug_layer = DebugLayer::ENABLE_WITH_GPU_VALIDATION;
	static const constexpr char* default_shader_model = "6.3";
	static const constexpr std::uint8_t num_back_buffers = 3;
	static const constexpr std::uint32_t num_instances_per_batch = 768U;		//48 KiB for ObjectData[]
	static const constexpr std::uint32_t num_lights = 21'845;					//1 MiB for StructuredBuffer<Light>
	static const constexpr bool use_bundles = false;
	static const constexpr bool use_exec_indirect = false;
	static const constexpr bool force_dxr_fallback = true;
	static const constexpr bool disable_rtx = false;
	static const constexpr unsigned int num_max_rt_materials = 10;
	static const constexpr unsigned int fallback_ptrs_offset = 25;

} /* wr::d3d12::settings */

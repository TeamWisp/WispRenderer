#pragma once

#include <d3d12.h>
#include <cstdint>
#include <dxgi1_5.h>

#include "d3d12_enums.hpp"

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

	static const constexpr bool output_hdr = false;
	static const constexpr Format back_buffer_format = output_hdr ? Format::R16G16B16A16_FLOAT : Format::R8G8B8A8_UNORM;
	static const constexpr DXGI_SWAP_EFFECT flip_mode = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	static const constexpr DXGI_SWAP_CHAIN_FLAG swapchain_flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	static const constexpr DXGI_SCALING swapchain_scaling = DXGI_SCALING_STRETCH;
	static const constexpr DXGI_ALPHA_MODE swapchain_alpha_mode = DXGI_ALPHA_MODE_UNSPECIFIED;
	static const constexpr bool enable_gpu_timeout = false;
	static const constexpr bool enable_debug_factory = true;
	static const constexpr DebugLayer enable_debug_layer = DebugLayer::ENABLE;	//Don't use ENABLE_WITH_GPU_VALIDATION (Raytracing); it breaks
	static const constexpr char* default_shader_model = "6_3";
	static std::array<LPCWSTR, 1> debug_shader_args = { L"/O3" };
	static std::array<LPCWSTR, 1> release_shader_args = { L"/O3" };
	static const constexpr std::uint8_t num_back_buffers = 3;
	static const constexpr std::uint32_t num_instances_per_batch = 768U;		//48 KiB for ObjectData[]
	static const constexpr std::uint32_t num_lights = 21'845;					//1 MiB for StructuredBuffer<Light>
	static const constexpr std::uint32_t num_indirect_draw_commands = 8;		//Allow 8 different meshes non-indexed
	static const constexpr std::uint32_t num_indirect_index_commands = 32;		//Allow 32 different meshes indexed
	static const constexpr bool use_bundles = false;
	static const constexpr bool force_dxr_fallback = false;
	static const constexpr bool disable_rtx = false;
	static const constexpr bool enable_object_culling = true;
	static const constexpr unsigned int num_max_rt_materials = 3000;
	static const constexpr unsigned int num_max_rt_textures = 1000;
	static const constexpr unsigned int fallback_ptrs_offset = 3500;
	static const constexpr std::uint32_t res_skybox = 1024;
	static const constexpr std::uint32_t res_envmap = 512;
	static const constexpr unsigned int shadow_denoiser_wavelet_iterations = 4;
	static const constexpr unsigned int shadow_denoiser_feedback_tap = 1;
	
} /* wr::d3d12::settings */

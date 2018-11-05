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
	static const constexpr bool enable_gpu_timeout = false;
	static const constexpr bool enable_debug_factory = false;
	static const DebugLayer enable_debug_layer = DebugLayer::ENABLE_WITH_GPU_VALIDATION;
	static const constexpr char* default_shader_model = "5.0";

} /* d3d12::settings */
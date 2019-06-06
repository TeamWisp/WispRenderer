#pragma once

#include "frame_graph/frame_graph.hpp"
#include "render_tasks/d3d12_imgui_render_task.hpp"
#include "render_tasks/d3d12_brdf_lut_precalculation.hpp"
#include "render_tasks/d3d12_deferred_main.hpp"
#include "render_tasks/d3d12_deferred_composition.hpp"
#include "render_tasks/d3d12_deferred_render_target_copy.hpp"
#include "render_tasks/d3d12_raytracing_task.hpp"
#include "render_tasks/d3d12_equirect_to_cubemap.hpp"
#include "render_tasks/d3d12_cubemap_convolution.hpp"
#include "render_tasks/d3d12_post_processing.hpp"
#include "render_tasks/d3d12_build_acceleration_structures.hpp"
#include "render_tasks/d3d12_path_tracer.hpp"
#include "render_tasks/d3d12_accumulation.hpp"
#include "render_tasks/d3d12_hbao.hpp"
#include "render_tasks/d3d12_ansel.hpp"

using frame_graph_setup_func_t = std::function<std::shared_ptr<wr::FrameGraph>(wr::D3D12RenderSystem&)>;

enum class FGType : std::int32_t
{
	PBR_BASIC,
	PBR_RT_REF_SHADOWS,
	PBR_RT_REF_SHADOWS_PATH_TRACING,
	COUNT
};

inline std::string GetFrameGraphName(FGType id)
{
	switch (id)
	{
	case FGType::PBR_BASIC: return "PBR Basic";
	case FGType::PBR_RT_REF_SHADOWS: return "PBR RT Reflections and Shadows";
	case FGType::PBR_RT_REF_SHADOWS_PATH_TRACING: return "PBR RT Reflections Shadows and Path Tracing";
	default:
		return "Unknown";
	}
}

static std::array<frame_graph_setup_func_t, static_cast<std::size_t>(FGType::COUNT)> frame_graph_setup_functions =
{
	// PBR Basic
	[] (wr::D3D12RenderSystem & rs)
	{
		auto fg = std::make_shared<wr::FrameGraph>(7);

		wr::AddBrdfLutPrecalculationTask(*fg);
		wr::AddEquirectToCubemapTask(*fg);
		wr::AddCubemapConvolutionTask(*fg);
		wr::AddDeferredMainTask(*fg, std::nullopt, std::nullopt, false);
		wr::AddDeferredCompositionTask(*fg, std::nullopt, std::nullopt);
		wr::AddPostProcessingTask<wr::DeferredCompositionTaskData>(*fg);
		wr::AddRenderTargetCopyTask<wr::PostProcessingData>(*fg);

		fg->Validate();
		fg->Setup(rs);
		return fg;
	},

	// PBR RT Reflections and shadows
	[](wr::D3D12RenderSystem& rs)
	{
		auto fg = std::make_shared<wr::FrameGraph>(7);

		wr::AddBrdfLutPrecalculationTask(*fg);
		wr::AddEquirectToCubemapTask(*fg);
		wr::AddCubemapConvolutionTask(*fg);
		wr::AddDeferredMainTask(*fg, std::nullopt, std::nullopt, true);
		wr::AddBuildAccelerationStructuresTask(*fg);
		wr::AddRTHybridTask(*fg);
		wr::AddDeferredCompositionTask(*fg, std::nullopt, std::nullopt);
		wr::AddPostProcessingTask<wr::DeferredCompositionTaskData>(*fg);
		wr::AddRenderTargetCopyTask<wr::PostProcessingData>(*fg);

		fg->Validate();
		fg->Setup(rs);
		return fg;
	},

	// PBR RT Reflections shadows and path tracing
	[](wr::D3D12RenderSystem& rs)
	{
		auto fg = std::make_shared<wr::FrameGraph>(7);

		wr::AddBrdfLutPrecalculationTask(*fg);
		wr::AddEquirectToCubemapTask(*fg);
		wr::AddCubemapConvolutionTask(*fg);
		wr::AddDeferredMainTask(*fg, std::nullopt, std::nullopt, true);
		wr::AddBuildAccelerationStructuresTask(*fg);
		wr::AddRTHybridTask(*fg);
		wr::AddPathTracerTask(*fg);
		wr::AddAccumulationTask<wr::PathTracerData>(*fg);
		wr::AddDeferredCompositionTask(*fg, std::nullopt, std::nullopt);
		wr::AddPostProcessingTask<wr::DeferredCompositionTaskData>(*fg);
		wr::AddRenderTargetCopyTask<wr::PostProcessingData>(*fg);

		fg->Validate();
		fg->Setup(rs);
		return fg;
	},
};

static std::shared_ptr<wr::FrameGraph> CreateFrameGraph(FGType type, wr::D3D12RenderSystem& rs)
{
	return frame_graph_setup_functions[static_cast<std::int32_t>(type)](rs);
}
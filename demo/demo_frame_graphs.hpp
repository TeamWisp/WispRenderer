#pragma once

#include "frame_graph/frame_graph.hpp"
#include "render_tasks/d3d12_imgui_render_task.hpp"
#include "render_tasks/d3d12_brdf_lut_precalculation.hpp"
#include "render_tasks/d3d12_deferred_main.hpp"
#include "render_tasks/d3d12_deferred_composition.hpp"
#include "render_tasks/d3d12_deferred_render_target_copy.hpp"
#include "render_tasks/d3d12_raytracing_task.hpp"
#include "render_tasks/d3d12_rt_hybrid_task.hpp"
#include "render_tasks/d3d12_rt_shadow_task.hpp"
#include "render_tasks/d3d12_rt_reflection_task.hpp"
#include "render_tasks/d3d12_equirect_to_cubemap.hpp"
#include "render_tasks/d3d12_cubemap_convolution.hpp"
#include "render_tasks/d3d12_shadow_denoiser_task.hpp"
#include "resources.hpp"
#include "render_tasks/d3d12_post_processing.hpp"
#include "render_tasks/d3d12_pixel_data_readback.hpp"
#include "render_tasks/d3d12_build_acceleration_structures.hpp"

namespace fg_manager
{

	enum class PrebuildFrameGraph
	{
		RAYTRACING = 0,
		DEFERRED = 1,
		RT_HYBRID = 2,
	};

	inline std::string GetFrameGraphName(PrebuildFrameGraph id)
	{
		switch (id)
		{
		case PrebuildFrameGraph::RAYTRACING:
			return "Full Raytracing";
		case PrebuildFrameGraph::DEFERRED:
			return "Deferred";
		case PrebuildFrameGraph::RT_HYBRID:
			return "Hybrid";
		default:
			return "Unknown";
		}
	}

	static PrebuildFrameGraph current = fg_manager::PrebuildFrameGraph::RT_HYBRID;
	static std::array<wr::FrameGraph*, 3> frame_graphs = {};

	inline void Setup(wr::RenderSystem& rs, util::Delegate<void(ImTextureID)> imgui_func)
	{
		// Raytracing
		{
			auto& fg = frame_graphs[(int)PrebuildFrameGraph::RAYTRACING];
			fg = new wr::FrameGraph(4);

			wr::AddBuildAccelerationStructuresTask(*fg);
			wr::AddEquirectToCubemapTask(*fg);
			wr::AddCubemapConvolutionTask(*fg);
			wr::AddRaytracingTask(*fg);
			wr::AddPostProcessingTask<wr::RaytracingData>(*fg);
			
			// Copy the scene render pixel data to the final render target
			wr::AddRenderTargetCopyTask<wr::PostProcessingData>(*fg);

			// Display ImGui
			fg->AddTask<wr::ImGuiTaskData>(wr::GetImGuiTask<wr::PostProcessingData>(imgui_func));

			fg->Setup(rs);
		}

		// Deferred
		{
			auto& fg = frame_graphs[(int)PrebuildFrameGraph::DEFERRED];
			fg = new wr::FrameGraph(7);
			
			wr::AddBrdfLutPrecalculationTask(*fg);
			wr::AddEquirectToCubemapTask(*fg);
			wr::AddCubemapConvolutionTask(*fg);
			wr::AddDeferredMainTask(*fg, std::nullopt, std::nullopt);
			wr::AddDeferredCompositionTask(*fg, std::nullopt, std::nullopt);

			// Do some post processing
			wr::AddPostProcessingTask<wr::DeferredCompositionTaskData>(*fg);

			// Copy the composition pixel data to the final render target
			wr::AddRenderTargetCopyTask<wr::PostProcessingData>(*fg);

			// Display ImGui
			fg->AddTask<wr::ImGuiTaskData>(wr::GetImGuiTask<wr::PostProcessingData>(imgui_func));

			fg->Setup(rs);
		}

		// Hybrid raytracing
		{
			auto& fg = frame_graphs[(int) PrebuildFrameGraph::RT_HYBRID];
			fg = new wr::FrameGraph(10);

			// Precalculate BRDF Lut
			wr::AddBrdfLutPrecalculationTask(*fg);

			wr::AddEquirectToCubemapTask(*fg);
			wr::AddCubemapConvolutionTask(*fg);

			 // Construct the G-buffer
			wr::AddDeferredMainTask(*fg, std::nullopt, std::nullopt);

			// Build Acceleration Structure
			wr::AddBuildAccelerationStructuresTask(*fg);

			// Raytracing task
			//wr::AddRTHybridTask(*fg);

			wr::AddRTReflectionTask(*fg);

			wr::AddRTShadowTask(*fg);

			wr::AddShadowDenoiserTask(*fg, std::nullopt);

			wr::AddDeferredCompositionTask(*fg, std::nullopt, std::nullopt);

			// Do some post processing
			wr::AddPostProcessingTask<wr::DeferredCompositionTaskData>(*fg);

			// Copy the raytracing pixel data to the final render target
			wr::AddRenderTargetCopyTask<wr::PostProcessingData>(*fg);

			// Display ImGui
			fg->AddTask<wr::ImGuiTaskData>(wr::GetImGuiTask<wr::PostProcessingData>(imgui_func));

			// Finalize the frame graph
			fg->Setup(rs);
		}
	}

	void Resize(wr::RenderSystem& render_system, std::uint32_t width, std::uint32_t height)
	{
		for (int i = 0; i < frame_graphs.size(); ++i)
		{
			frame_graphs[i]->Resize(render_system, width, height);
		}
	}

	inline wr::FrameGraph* Get()
	{
		return frame_graphs[(int)current];
	}

	inline void Next()
	{
		current = (PrebuildFrameGraph)(((int)current + 1) % frame_graphs.size());
	}

	inline void Set(PrebuildFrameGraph value)
	{
		current = value;
	}

	inline void Destroy()
	{
		for (auto& fg : frame_graphs)
		{
			delete fg;
		}
	}
}
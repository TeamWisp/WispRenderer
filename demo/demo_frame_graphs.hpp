#pragma once

#include "frame_graph/frame_graph.hpp"
#include "render_tasks/d3d12_imgui_render_task.hpp"
#include "render_tasks/d3d12_brdf_lut_precalculation.hpp"
#include "render_tasks/d3d12_deferred_main.hpp"
#include "render_tasks/d3d12_deferred_composition.hpp"
#include "render_tasks/d3d12_deferred_render_target_copy.hpp"
#include "render_tasks/d3d12_raytracing_task.hpp"
#include "render_tasks/d3d12_rt_hybrid_task.hpp"
#include "render_tasks/d3d12_equirect_to_cubemap.hpp"
#include "render_tasks/d3d12_cubemap_convolution.hpp"
#include "render_tasks/d3d12_post_processing.hpp"
#include "render_tasks/d3d12_pixel_data_readback.hpp"
#include "render_tasks/d3d12_build_acceleration_structures.hpp"
#include "render_tasks/d3d12_path_tracer.hpp"
#include "render_tasks/d3d12_accumulation.hpp"
#include "render_tasks/d3d12_dof_bokeh.hpp"
#include "render_tasks/d3d12_dof_bokeh_postfilter.hpp"
#include "render_tasks/d3d12_dof_coc.hpp"
#include "render_tasks/d3d12_dof_down_scale.hpp"
#include "render_tasks/d3d12_dof_composition.hpp"
#include "render_tasks/d3d12_dof_dilate_near.hpp"
#include "render_tasks/d3d12_dof_dilate_flatten.hpp"
#include "render_tasks/d3d12_dof_dilate_flatten_second_pass.hpp"
#include "render_tasks/d3d12_hbao.hpp"
#include "render_tasks/d3d12_ansel.hpp"

namespace fg_manager
{

	enum class PrebuildFrameGraph
	{
		DEFERRED = 0,
		RT_HYBRID = 1,
		RAYTRACING = 2,
		PATH_TRACER = 3,
	};

	inline std::string GetFrameGraphName(PrebuildFrameGraph id)
	{
		switch (id)
		{
		case PrebuildFrameGraph::DEFERRED:
			return "Deferred";
		case PrebuildFrameGraph::RT_HYBRID:
			return "Hybrid";
		case PrebuildFrameGraph::RAYTRACING:
			return "Full Raytracing";
		case PrebuildFrameGraph::PATH_TRACER:
			return "Path Tracer";
		default:
			return "Unknown";
		}
	}

	static PrebuildFrameGraph current = fg_manager::PrebuildFrameGraph::DEFERRED;
	static std::array<wr::FrameGraph*, 4> frame_graphs = {};

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
			fg = new wr::FrameGraph(15);
			
			wr::AddBrdfLutPrecalculationTask(*fg);
			wr::AddEquirectToCubemapTask(*fg);
			wr::AddCubemapConvolutionTask(*fg);
			wr::AddDeferredMainTask(*fg, std::nullopt, std::nullopt);
			wr::AddHBAOTask(*fg);
			wr::AddDeferredCompositionTask(*fg, std::nullopt, std::nullopt);

			// Do some post processing
			wr::AddPostProcessingTask<wr::DeferredCompositionTaskData>(*fg);
				
			// Do Depth of field task
			wr::AddDoFCoCTask<wr::DeferredMainTaskData>(*fg);

			wr::AddDoFDownScaleTask<wr::PostProcessingData, wr::DoFCoCData>(*fg,
				rs.m_window.value()->GetWidth(), rs.m_window.value()->GetHeight());

			wr::AddDoFDilateTask<wr::DoFDownScaleData>(*fg,
				rs.m_window.value()->GetWidth(), rs.m_window.value()->GetHeight());

			wr::AddDoFDilateFlattenTask<wr::DoFDilateData>(*fg,
				rs.m_window.value()->GetWidth(), rs.m_window.value()->GetHeight());

			wr::AddDoFDilateFlattenHTask<wr::DoFDilateFlattenData>(*fg,
				rs.m_window.value()->GetWidth(), rs.m_window.value()->GetHeight());

			wr::AddDoFBokehTask<wr::DoFDownScaleData, wr::DoFDilateFlattenHData>(*fg,
				rs.m_window.value()->GetWidth(), rs.m_window.value()->GetHeight());

			wr::AddDoFBokehPostFilterTask<wr::DoFBokehData>(*fg,
				rs.m_window.value()->GetWidth(), rs.m_window.value()->GetHeight());

			wr::AddDoFCompositionTask<wr::PostProcessingData, wr::DoFBokehPostFilterData, wr::DoFCoCData>(*fg);

			// Copy the scene render pixel data to the final render target
			wr::AddRenderTargetCopyTask<wr::DoFCompositionData>(*fg);
			wr::AddAnselTask(*fg);
			// Display ImGui
			fg->AddTask<wr::ImGuiTaskData>(wr::GetImGuiTask<wr::DoFCompositionData>(imgui_func));

			fg->Setup(rs);
		}

		// Path Tracer
		{
			auto& fg = frame_graphs[(int)PrebuildFrameGraph::PATH_TRACER];
			fg = new wr::FrameGraph(10);

			// Precalculate BRDF Lut
			wr::AddBrdfLutPrecalculationTask(*fg);

			wr::AddEquirectToCubemapTask(*fg);
			wr::AddCubemapConvolutionTask(*fg);

			// Construct the G-buffer
			wr::AddDeferredMainTask(*fg, std::nullopt, std::nullopt);

			wr::AddHBAOTask(*fg);

			// Build Acceleration Structure
			wr::AddBuildAccelerationStructuresTask(*fg);

			// Raytracing task
			//wr::AddRTHybridTask(*fg);

			// Global Illumination Path Tracing
			wr::AddPathTracerTask(*fg);
			wr::AddAccumulationTask<wr::PathTracerData>(*fg);

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

		// Hybrid raytracing
		{
			auto& fg = frame_graphs[(int) PrebuildFrameGraph::RT_HYBRID];
			fg = new wr::FrameGraph(15);

			// Precalculate BRDF Lut
			wr::AddBrdfLutPrecalculationTask(*fg);

			wr::AddEquirectToCubemapTask(*fg);
			wr::AddCubemapConvolutionTask(*fg);

			 // Construct the G-buffer
			wr::AddDeferredMainTask(*fg, std::nullopt, std::nullopt);

			// Build Acceleration Structure
			wr::AddBuildAccelerationStructuresTask(*fg);

			// Raytracing task
			wr::AddRTHybridTask(*fg);

			wr::AddDeferredCompositionTask(*fg, std::nullopt, std::nullopt);

			// Do some post processing
			wr::AddPostProcessingTask<wr::DeferredCompositionTaskData>(*fg);

			// Do Depth of field task
			wr::AddDoFCoCTask<wr::DeferredMainTaskData>(*fg);

			wr::AddDoFDownScaleTask<wr::PostProcessingData, wr::DoFCoCData>(*fg,
				rs.m_window.value()->GetWidth(), rs.m_window.value()->GetHeight());

			wr::AddDoFDilateTask<wr::DoFDownScaleData>(*fg,
				rs.m_window.value()->GetWidth(), rs.m_window.value()->GetHeight());

			wr::AddDoFDilateFlattenTask<wr::DoFDilateData>(*fg,
				rs.m_window.value()->GetWidth(), rs.m_window.value()->GetHeight());

			wr::AddDoFDilateFlattenHTask<wr::DoFDilateFlattenData>(*fg,
				rs.m_window.value()->GetWidth(), rs.m_window.value()->GetHeight());

			wr::AddDoFBokehTask<wr::DoFDownScaleData, wr::DoFDilateFlattenHData>(*fg,
				rs.m_window.value()->GetWidth(), rs.m_window.value()->GetHeight());

			wr::AddDoFBokehPostFilterTask<wr::DoFBokehData>(*fg,
				rs.m_window.value()->GetWidth(), rs.m_window.value()->GetHeight());

			wr::AddDoFCompositionTask<wr::PostProcessingData, wr::DoFBokehPostFilterData, wr::DoFCoCData>(*fg);

			// Copy the scene render pixel data to the final render target
			wr::AddRenderTargetCopyTask<wr::DoFCompositionData>(*fg);
			// Display ImGui
			fg->AddTask<wr::ImGuiTaskData>(wr::GetImGuiTask<wr::DoFCompositionData>(imgui_func));

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

	inline void Prev()
	{
		current = (PrebuildFrameGraph)(((int)current - 1) % frame_graphs.size());
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

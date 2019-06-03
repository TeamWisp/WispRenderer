#pragma once

#include "frame_graph/frame_graph.hpp"
#include "settings.hpp"
#include "render_tasks/d3d12_imgui_render_task.hpp"
#include "render_tasks/d3d12_brdf_lut_precalculation.hpp"
#include "render_tasks/d3d12_deferred_main.hpp"
#include "render_tasks/d3d12_deferred_composition.hpp"
#include "render_tasks/d3d12_deferred_render_target_copy.hpp"
#include "render_tasks/d3d12_raytracing_task.hpp"
#include "render_tasks/d3d12_rt_hybrid_task.hpp"
#include "render_tasks/d3d12_rt_reflection_task.hpp"
#include "render_tasks/d3d12_rt_shadow_task.hpp"
#include "render_tasks/d3d12_shadow_denoiser_task.hpp"
#include "render_tasks/d3d12_equirect_to_cubemap.hpp"
#include "render_tasks/d3d12_cubemap_convolution.hpp"
#include "render_tasks/d3d12_rtao_task.hpp"
#include "render_tasks/d3d12_post_processing.hpp"
#include "render_tasks/d3d12_build_acceleration_structures.hpp"
#include "render_tasks/d3d12_path_tracer.hpp"
#include "render_tasks/d3d12_accumulation.hpp"
#include "render_tasks/d3d12_dof_bokeh.hpp"
#include "render_tasks/d3d12_dof_bokeh_postfilter.hpp"
#include "render_tasks/d3d12_dof_coc.hpp"
#include "render_tasks/d3d12_down_scale.hpp"
#include "render_tasks/d3d12_dof_composition.hpp"
#include "render_tasks/d3d12_dof_dilate_near.hpp"
#include "render_tasks/d3d12_hbao.hpp"
#include "render_tasks/d3d12_ansel.hpp"
#include "render_tasks/d3d12_bloom_extract_bright.hpp"
#include "render_tasks/d3d12_bloom_composition.hpp"
#include "render_tasks/d3d12_bloom_half_res.hpp"
#include "render_tasks/d3d12_bloom_half_res_v.hpp"
#include "render_tasks/d3d12_bloom_quarter_res.hpp"
#include "render_tasks/d3d12_bloom_quarter_res_v.hpp"
#include "render_tasks/d3d12_bloom_eighth_res.hpp"
#include "render_tasks/d3d12_bloom_eighth_res_v.hpp"
#include "render_tasks/d3d12_bloom_sixteenth_res.hpp"
#include "render_tasks/d3d12_bloom_sixteenth_res_v.hpp"


namespace fg_manager
{

	enum class PrebuildFrameGraph : std::uint32_t
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

	inline void Setup(wr::RenderSystem& rs, util::Delegate<void(ImTextureID)> const& imgui_func)
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
			fg->AddTask<wr::ImGuiTaskData>(wr::GetImGuiTask<wr::PostProcessingData>(imgui_func), L"ImGui");

			fg->Setup(rs);
		}

		// Deferred
		{
			auto& fg = frame_graphs[(int)PrebuildFrameGraph::DEFERRED];
			fg = new wr::FrameGraph(24);
			
			wr::AddBrdfLutPrecalculationTask(*fg);
			wr::AddEquirectToCubemapTask(*fg);
			wr::AddCubemapConvolutionTask(*fg);
			wr::AddDeferredMainTask(*fg, std::nullopt, std::nullopt, false);
			wr::AddHBAOTask(*fg);
			wr::AddDeferredCompositionTask(*fg, std::nullopt, std::nullopt);

			//High quality bloom pass
			wr::AddBloomExtractBrightTask<wr::DeferredCompositionTaskData, wr::DeferredMainTaskData>(*fg);
			wr::AddBloomHalfTask<wr::BloomExtractBrightData>(*fg);
			wr::AddBloomHalfVTask<wr::BloomHalfData>(*fg);
			wr::AddBloomQuarterTask<wr::BloomExtractBrightData>(*fg);
			wr::AddBloomQuarterVTask<wr::BloomQuarterData>(*fg);
			wr::AddBloomEighthTask<wr::BloomExtractBrightData>(*fg);
			wr::AddBloomEighthVTask<wr::BloomEighthData>(*fg);
			wr::AddBloomSixteenthTask<wr::BloomExtractBrightData>(*fg);
			wr::AddBloomSixteenthVTask<wr::BloomSixteenthData>(*fg);
			wr::AddBloomCompositionTask<wr::DeferredCompositionTaskData, wr::BloomHalfVData, wr::BloomQuarterVData, wr::BloomEighthVData, wr::BloomSixteenthVData>(*fg);

			// Do Depth of field task
			wr::AddDoFCoCTask<wr::DeferredMainTaskData>(*fg);
			wr::AddDownScaleTask<wr::BloomCompostionData, wr::DoFCoCData>(*fg);
			wr::AddDoFDilateTask<wr::DownScaleData>(*fg);
			wr::AddDoFBokehTask<wr::DownScaleData, wr::DoFDilateData>(*fg);
			wr::AddDoFBokehPostFilterTask<wr::DoFBokehData>(*fg);

			wr::AddDoFCompositionTask<wr::BloomCompostionData, wr::DoFBokehPostFilterData, wr::DoFCoCData>(*fg);

			wr::AddPostProcessingTask<wr::DoFCompositionData>(*fg);

			// Copy the scene render pixel data to the final render target
			wr::AddRenderTargetCopyTask<wr::PostProcessingData>(*fg);

			wr::AddAnselTask(*fg);

			// Display ImGui
			fg->AddTask<wr::ImGuiTaskData>(wr::GetImGuiTask<wr::PostProcessingData>(imgui_func), L"ImGui");

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
			wr::AddDeferredMainTask(*fg, std::nullopt, std::nullopt, true);

			wr::AddHBAOTask(*fg);

			// Build Acceleration Structure
			wr::AddBuildAccelerationStructuresTask(*fg);

			// Raytracing task
			wr::AddRTHybridTask(*fg);

			// Global Illumination Path Tracing
			wr::AddPathTracerTask(*fg);
			wr::AddAccumulationTask<wr::PathTracerData>(*fg);

			wr::AddDeferredCompositionTask(*fg, std::nullopt, std::nullopt);

			// Do some post processing
			wr::AddPostProcessingTask<wr::DeferredCompositionTaskData>(*fg);

			// Copy the raytracing pixel data to the final render target
			wr::AddRenderTargetCopyTask<wr::PostProcessingData>(*fg);

			wr::AddAnselTask(*fg);

			// Display ImGui
			fg->AddTask<wr::ImGuiTaskData>(wr::GetImGuiTask<wr::PostProcessingData>(imgui_func), L"ImGui");

			// Finalize the frame graph
			fg->Setup(rs);
		}

		// Hybrid raytracing
		{
			auto& fg = frame_graphs[(int) PrebuildFrameGraph::RT_HYBRID];
			fg = new wr::FrameGraph(27);

			// Precalculate BRDF Lut
			wr::AddBrdfLutPrecalculationTask(*fg);

			wr::AddEquirectToCubemapTask(*fg);
			wr::AddCubemapConvolutionTask(*fg);
			 // Construct the G-buffer
			wr::AddDeferredMainTask(*fg, std::nullopt, std::nullopt, true);

			// Build Acceleration Structure
			wr::AddBuildAccelerationStructuresTask(*fg);

			// Raytracing task
			wr::AddRTReflectionTask(*fg);
			wr::AddRTShadowTask(*fg);

			wr::AddShadowDenoiserTask(*fg);

			//Raytraced Ambient Occlusion task
			wr::AddRTAOTask(*fg, static_cast<wr::D3D12RenderSystem&>(rs).m_device);

			wr::AddDeferredCompositionTask(*fg, std::nullopt, std::nullopt);

			//High quality bloom pass
			wr::AddBloomExtractBrightTask<wr::DeferredCompositionTaskData, wr::DeferredMainTaskData>(*fg);
			wr::AddBloomHalfTask<wr::BloomExtractBrightData>(*fg);
			wr::AddBloomHalfVTask<wr::BloomHalfData>(*fg);
			wr::AddBloomQuarterTask<wr::BloomExtractBrightData>(*fg);
			wr::AddBloomQuarterVTask<wr::BloomQuarterData>(*fg);
			wr::AddBloomEighthTask<wr::BloomExtractBrightData>(*fg);
			wr::AddBloomEighthVTask<wr::BloomEighthData>(*fg);
			wr::AddBloomSixteenthTask<wr::BloomExtractBrightData>(*fg);
			wr::AddBloomSixteenthVTask<wr::BloomSixteenthData>(*fg);
			wr::AddBloomCompositionTask<wr::DeferredCompositionTaskData, wr::BloomHalfVData, wr::BloomQuarterVData, wr::BloomEighthVData, wr::BloomSixteenthVData>(*fg);

			// Do Depth of field task
			wr::AddDoFCoCTask<wr::DeferredMainTaskData>(*fg);
			wr::AddDownScaleTask<wr::BloomCompostionData, wr::DoFCoCData>(*fg);
			wr::AddDoFDilateTask<wr::DownScaleData>(*fg);
			wr::AddDoFBokehTask<wr::DownScaleData, wr::DoFDilateData>(*fg);
			wr::AddDoFBokehPostFilterTask<wr::DoFBokehData>(*fg);
			wr::AddDoFCompositionTask<wr::BloomCompostionData, wr::DoFBokehPostFilterData, wr::DoFCoCData>(*fg);

			wr::AddPostProcessingTask<wr::DoFCompositionData>(*fg);

			// Copy the scene render pixel data to the final render target
			wr::AddRenderTargetCopyTask<wr::PostProcessingData>(*fg);
			// Display ImGui
			fg->AddTask<wr::ImGuiTaskData>(wr::GetImGuiTask<wr::PostProcessingData>(imgui_func), L"ImGui");

			// Finalize the frame graph
			fg->Setup(rs);
		}
	}

	void Resize(wr::RenderSystem& render_system, std::uint32_t width, std::uint32_t height)
	{
		for (int i = 0; i < frame_graphs.size(); ++i)
		{
			frame_graphs[i]->Resize(width, height);
		}
	}

	inline wr::FrameGraph* Get()
	{
		return frame_graphs[(int)current];
	}

	inline void Next()
	{
		current = (PrebuildFrameGraph)((static_cast<std::uint32_t>(current) + 1ull) % frame_graphs.size());
	}

	inline void Prev()
	{
		current = (PrebuildFrameGraph)((static_cast<std::uint32_t>(current) - 1ull) % frame_graphs.size());
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

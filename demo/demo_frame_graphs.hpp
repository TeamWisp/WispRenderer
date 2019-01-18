#pragma once

#include "frame_graph/frame_graph.hpp"
#include "render_tasks/d3d12_imgui_render_task.hpp"
#include "render_tasks/d3d12_deferred_main.hpp"
#include "render_tasks/d3d12_deferred_composition.hpp"
#include "render_tasks/d3d12_deferred_render_target_copy.hpp"
#include "render_tasks/d3d12_raytracing_task.hpp"
#include "render_tasks/d3d12_rt_hybrid_task.hpp"
#include "render_tasks/d3d12_accumulation.hpp"
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

	static PrebuildFrameGraph current = fg_manager::PrebuildFrameGraph::RT_HYBRID;
	static std::array<wr::FrameGraph*, 3> frame_graphs = {};

	inline void Setup(wr::RenderSystem& rs, util::Delegate<void()> imgui_func)
	{
		// Ray tracing
		{
			auto& fg = frame_graphs[(int)PrebuildFrameGraph::RAYTRACING];
			fg = new wr::FrameGraph(3);

			wr::AddBuildAccelerationStructuresTask(*fg);
			wr::AddRaytracingTask(*fg);
			wr::AddAccumulationTask(*fg);
			
			// Copy the scene render pixel data to the final render target
			wr::AddRenderTargetCopyTask<wr::AccumulationData>(*fg);

			// Display ImGui
			fg->AddTask<wr::ImGuiTaskData>(wr::GetImGuiTask(imgui_func));

			// Finalize the frame graph
			fg->Setup(rs);
		}

		// Deferred
		{
			auto& fg = frame_graphs[(int)PrebuildFrameGraph::DEFERRED];
			fg = new wr::FrameGraph(4);

			// Construct the G-buffer
			wr::AddDeferredMainTask(*fg, std::nullopt, std::nullopt);

			// Merge the G-buffer into one final texture
			wr::AddDeferredCompositionTask(*fg, std::nullopt, std::nullopt);

			// Copy the composition pixel data to the final render target
			wr::AddRenderTargetCopyTask<wr::DeferredCompositionTaskData>(*fg);

			// Display ImGui
			fg->AddTask<wr::ImGuiTaskData>(wr::GetImGuiTask(imgui_func));

			// Finalize the frame graph
			fg->Setup(rs);
		}

		// Hybrid raytracing
		{
			auto& fg = frame_graphs[(int) PrebuildFrameGraph::RT_HYBRID];
			fg = new wr::FrameGraph(5);

			 // Construct the G-buffer
			wr::AddDeferredMainTask(*fg, std::nullopt, std::nullopt);

			// Build Acceleration Structure
			wr::AddBuildAccelerationStructuresTask(*fg);

			// Raytracing task
			wr::AddRTHybridTask(*fg);

			// Copy the raytracing pixel data to the final render target
			wr::AddRenderTargetCopyTask<wr::RTHybridData>(*fg);

			// Display ImGui
			fg->AddTask<wr::ImGuiTaskData>(wr::GetImGuiTask(imgui_func));

			// Finalize the frame graph
			fg->Setup(rs);
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

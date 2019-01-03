#pragma once

#include "frame_graph/frame_graph.hpp"
#include "render_tasks/d3d12_imgui_render_task.hpp"
#include "render_tasks/d3d12_deferred_main.hpp"
#include "render_tasks/d3d12_deferred_composition.hpp"
#include "render_tasks/d3d12_deferred_render_target_copy.hpp"
#include "render_tasks/d3d12_raytracing_task.hpp"

namespace fg_manager
{

	enum class PrebuildFrameGraph
	{
		RAYTRACING = 0,
		DEFERRED = 1,
	};

	static PrebuildFrameGraph current = fg_manager::PrebuildFrameGraph::DEFERRED;
	static std::array<wr::FrameGraph*, 2> frame_graphs = {};

	inline void Setup(wr::RenderSystem& rs, util::Delegate<void()> imgui_func)
	{
		// Raytracing
		{
			auto& fg = frame_graphs[(int)PrebuildFrameGraph::RAYTRACING];
			fg = new wr::FrameGraph(3);

			wr::AddRaytracingTask(*fg);
			wr::AddRenderTargetCopyTask<wr::RaytracingData>(*fg);
			fg->AddTask<wr::ImGuiTaskData>(wr::GetImGuiTask(imgui_func));

			fg->Setup(rs);
		}

		// Deferred
		{
			auto& fg = frame_graphs[(int)PrebuildFrameGraph::DEFERRED];
			fg = new wr::FrameGraph(4);

			wr::AddDeferredMainTask(*fg);
			wr::AddDeferredCompositionTask(*fg);
			wr::AddRenderTargetCopyTask<wr::DeferredCompositionTaskData>(*fg);
			fg->AddTask<wr::ImGuiTaskData>(wr::GetImGuiTask(imgui_func));

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
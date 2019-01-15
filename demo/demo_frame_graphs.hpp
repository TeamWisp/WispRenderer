#pragma once

#include "frame_graph/frame_graph.hpp"
#include "render_tasks/d3d12_imgui_render_task.hpp"
#include "render_tasks/d3d12_deferred_main.hpp"
#include "render_tasks/d3d12_deferred_composition.hpp"
#include "render_tasks/d3d12_deferred_render_target_copy.hpp"
#include "render_tasks/d3d12_raytracing_task.hpp"
#include "render_tasks/d3d12_accumulation.hpp"
#include "render_tasks/d3d12_equirect_to_cubemap.hpp"
#include "render_tasks/d3d12_cubemap_convolution.hpp"
#include "resources.hpp"

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
			fg = new wr::FrameGraph(4);

			wr::AddBuildAccelerationStructuresTask(*fg);
			wr::AddRaytracingTask(*fg);
			wr::AddAccumulationTask(*fg);
			wr::AddRenderTargetCopyTask<wr::AccumulationData>(*fg);
			fg->AddTask<wr::ImGuiTaskData>(wr::GetImGuiTask(imgui_func));

			fg->Setup(rs);
		}

		// Deferred
		{
			auto& fg = frame_graphs[(int)PrebuildFrameGraph::DEFERRED];
			fg = new wr::FrameGraph(6);
			
			wr::AddEquirectToCubemapTask(*fg, resources::equirectangular_environment_map, resources::cubemap_environment_map);
			wr::AddCubemapConvolutionTask(*fg, resources::loaded_skybox, resources::convoluted_environment_map);
			wr::AddDeferredMainTask(*fg, std::nullopt, std::nullopt);
			wr::AddDeferredCompositionTask(*fg, std::nullopt, std::nullopt);
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
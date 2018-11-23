#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../frame_graph/render_task.hpp"
#include "../frame_graph/frame_graph.hpp"

namespace wr
{

	struct DeferredTaskData
	{	
		bool in_boolean;
		int out_integer;
	};
	
	namespace internal
	{

		inline void SetupDeferredTask(RenderSystem & render_system, RenderTask<DeferredTaskData> & task, DeferredTaskData & data)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);
		}

		inline void ExecuteDeferredTask(RenderSystem & render_system, RenderTask<DeferredTaskData> & task, SceneGraph & scene_graph, DeferredTaskData & data)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);
		}

	} /* internal */
	

	//! Used to create a new defferred task.
	[[nodiscard]] inline std::unique_ptr<RenderTask<DeferredTaskData>> GetDeferredTask()
	{
		auto ptr = std::make_unique<RenderTask<DeferredTaskData>>(nullptr, "Deferred Render Task", RenderTaskType::DIRECT, std::nullopt,
			[](RenderSystem & render_system, RenderTask<DeferredTaskData> & task, DeferredTaskData & data) { internal::SetupDeferredTask(render_system, task, data); },
			[](RenderSystem & render_system, RenderTask<DeferredTaskData> & task, SceneGraph & scene_graph, DeferredTaskData & data) { internal::ExecuteDeferredTask(render_system, task, scene_graph, data); });

		return ptr;
	}

} /* wr */
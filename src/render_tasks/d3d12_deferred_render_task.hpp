#pragma once

#include "../frame_graph/task.hpp"
#include "../resource.hpp"
#include "../frame_graph/frame_graph.hpp"

namespace wr::fg
{

	struct DeferredTaskData
	{
		bool in_boolean;
		int out_integer;
	};
	
	inline void SetupDeferredTask(RenderSystem & render_system, Task<DeferredTaskData> & task, DeferredTaskData & data)
	{
		auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);

		//int* i;
		//data.in_boolean = task.Create<Resource<bool>>("Some texture", i);
		//data.out_integer = task.Create<Resource<int>>("Out integer", i);
		//task.Create(&data.in_boolean, "Some texture", i);
		//task.Create(&data.out_integer, "Out integer", i);
	}

	inline void ExecuteDeferredTask(RenderSystem & render_system, Task<DeferredTaskData> & task, SceneGraph & scene_graph, DeferredTaskData & data)
	{
		auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);
	}
	
	inline std::unique_ptr<Task<DeferredTaskData>> GetDeferredTask()
	{
		auto ptr = std::make_unique<Task<DeferredTaskData>>(nullptr, "Deferred Render Task",
			[](RenderSystem & render_system, Task<DeferredTaskData> & task, DeferredTaskData & data) { SetupDeferredTask(render_system, task, data); },
			[](RenderSystem & render_system, Task<DeferredTaskData> & task, SceneGraph & scene_graph, DeferredTaskData & data) { ExecuteDeferredTask(render_system, task, scene_graph, data); });
		return ptr;
	}
}
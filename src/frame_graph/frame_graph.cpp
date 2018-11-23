#include "frame_graph.hpp"

namespace wr
{
	FrameGraph::~FrameGraph()
	{
		Destroy();
	}
	//! Adds a task without creating it.
	void FrameGraph::AddTask(std::unique_ptr<BaseRenderTask> task)
	{
		task->SetFrameGraph(this);
		m_tasks.push_back(std::move(task));
	}

	//! Setup all render tasks in order.
	void FrameGraph::Setup(RenderSystem & render_system)
	{
		for (auto& task : m_tasks)
		{
			task->Setup(render_system);
		}
	}

	//! Execute all render tasks in order.
	void FrameGraph::Execute(RenderSystem & render_system, SceneGraph & scene_graph)
	{
		for (auto& task : m_tasks)
		{
			task->Execute(render_system, scene_graph);
		}
	}

	void FrameGraph::Destroy()
	{
		for (auto& task : m_tasks)
		{
			task.reset();
		}
		m_tasks.clear();
	}

} /* wr */
#include "frame_graph.hpp"

namespace wr::fg
{

	void FrameGraph::AddTask(std::unique_ptr<BaseRenderTask> task)
	{
		task->SetFrameGraph(this);
		m_tasks.push_back(std::move(task));
	}

	void FrameGraph::Setup(RenderSystem & render_system)
	{
		for (auto& task : m_tasks)
		{
			task->Setup(render_system);
		}
	}

	void FrameGraph::Execute(RenderSystem & render_system, SceneGraph & scene_graph)
	{
		for (auto& task : m_tasks)
		{
			task->Execute(render_system, scene_graph);
		}
	}

} /* wr::fg */
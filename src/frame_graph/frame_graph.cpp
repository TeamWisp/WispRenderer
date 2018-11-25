#include "frame_graph.hpp"

#include "../settings.hpp"

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
			if (settings::use_multithreading && task->m_allow_multithreading)
			{
				task->WaitForCompletion();
				std::future<void> f = std::async(std::launch::async, [this, &task, &render_system]
				{
					task->Setup(render_system);
				});
				task->SetFuture(f);
			}
			else
			{
				non_multithreading:
				task->Setup(render_system);
			}
		}
	}

	//! Execute all render tasks in order.
	void FrameGraph::Execute(RenderSystem & render_system, SceneGraph & scene_graph)
	{
		for (auto& task : m_tasks)
		{
			if (settings::use_multithreading && task->m_allow_multithreading) // TODO: make constexpr.
			{
				task->WaitForCompletion();
				std::future<void> f = std::async(std::launch::async, [&]
				{
					task->Execute(render_system, scene_graph);
				});
				task->SetFuture(f);
			} else
			{
				task->Execute(render_system, scene_graph);
			}
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
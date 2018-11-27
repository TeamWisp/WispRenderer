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

		if (task->m_allow_multithreading)
		{
			m_multi_threaded_tasks.push_back(task.get());
		}
		else
		{
			m_single_threaded_tasks.push_back(task.get());
		}

		m_tasks.push_back(std::move(task));

	}

	//! Setup all render tasks in order.
	void FrameGraph::Setup(RenderSystem & render_system)
	{
		if constexpr (settings::use_multithreading)
		{
			// Multithreading behaviour
			auto num_mt_render_tasks = m_multi_threaded_tasks.size();
			constexpr auto num_threads = settings::num_frame_graph_threads;

			if (settings::num_frame_graph_threads > m_multi_threaded_tasks.size())
			{
				LOGW("The frame graph's thread pool has more threads than multi thread allowed tasks. This is allowed but not recommended.");
			}

			auto ThreadDivider = [&](unsigned int const thread_id)
			{
				const unsigned num_tasks = num_mt_render_tasks / settings::num_frame_graph_threads, num_tougher_threads = num_mt_render_tasks % settings::num_frame_graph_threads;
				for (unsigned int index0 = (thread_id < num_tougher_threads ? thread_id * (num_tasks + 1) : num_mt_render_tasks - (num_threads - thread_id) * num_tasks), index = index0; index < index0 + num_tasks + (thread_id < num_tougher_threads); ++index)
				{
					auto& task = m_multi_threaded_tasks[index];
					task->WaitForCompletion();
					auto f = m_thread_pool.Enqueue([this, &task, &render_system]
					{
						task->Setup(render_system);
					});
					task->SetFuture(f);
				}
			};

			for (auto i = 0u; i < num_threads; i++)
			{
				ThreadDivider(i);
			}

			// Singlethreading behaviour
			for (auto& task : m_single_threaded_tasks)
			{
				task->Setup(render_system);
			}
		}
		// No multithreading at all
		else
		{
			for (auto& task : m_tasks)
			{
				task->WaitForCompletion();
				task->Setup(render_system);
			}
		}
	}

	//! Execute all render tasks in order.
	void FrameGraph::Execute(RenderSystem & render_system, SceneGraph & scene_graph)
	{
		if constexpr (settings::use_multithreading)
		{
			// Multithreading behaviour
			auto num_mt_render_tasks = m_multi_threaded_tasks.size();
			constexpr auto num_threads = settings::num_frame_graph_threads;

			auto ThreadDivider = [&](unsigned int const thread_id)
			{
				const unsigned num_tasks = num_mt_render_tasks / settings::num_frame_graph_threads, num_tougher_threads = num_mt_render_tasks % settings::num_frame_graph_threads;
				for (unsigned int index0 = (thread_id < num_tougher_threads ? thread_id * (num_tasks + 1) : num_mt_render_tasks - (num_threads - thread_id) * num_tasks), index = index0; index < index0 + num_tasks + (thread_id < num_tougher_threads); ++index)
				{
					auto& task = m_multi_threaded_tasks[index];
					task->WaitForCompletion();
					auto f = m_thread_pool.Enqueue([this, &task, &render_system, &scene_graph]
					{
						task->Execute(render_system, scene_graph);
					});
					task->SetFuture(f);
				}
			};

			for (auto i = 0u; i < num_threads; i++)
			{
				ThreadDivider(i);
			}

			// Singlethreading behaviour
			for (auto& task : m_single_threaded_tasks)
			{
				task->Execute(render_system, scene_graph);
			}
		}
		// No multithreading at all
		else
		{
			for (auto& task : m_tasks)
			{
				task->WaitForCompletion();
				task->Execute(render_system, scene_graph);
			}
		}

		/*for (auto& task : m_tasks)
		{
			if (settings::use_multithreading && task->m_allow_multithreading) // TODO: make constexpr.
			{
				task->WaitForCompletion();
				auto f = m_thread_pool.Enqueue([this, &task, &render_system, &scene_graph]
				{
					task->Execute(render_system, scene_graph);
				});
				task->SetFuture(f);
			} else
			{
				task->Execute(render_system, scene_graph);
			}
		}*/
	}

	void FrameGraph::Resize(RenderSystem & render_system, std::uint32_t width, std::uint32_t height)
	{
		for (auto& task : m_tasks)
		{
			task->Resize(render_system, width, height);
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
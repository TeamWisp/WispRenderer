#pragma once

#include <functional>

#include "task.hpp"

namespace wr::fg
{

	class FrameGraph
	{
		friend class BaseTask;
	public:
		FrameGraph() {}
		virtual ~FrameGraph() = default;

		FrameGraph(const FrameGraph&) = delete;
		FrameGraph(FrameGraph&&) = default;
		FrameGraph& operator=(const FrameGraph&) = delete;
		FrameGraph& operator=(FrameGraph&&) = default;

		template<typename T>
		void AddTask(std::string const & name, decltype(Task<T>::setup_func_type) setup, decltype(Task<T>::execute_func_type) execute)
		{
			tasks.push_back(std::make_unique<Task<T>>(this, name, setup, execute));
		}

		void AddTask(std::unique_ptr<BaseTask> task)
		{
			task->SetFrameGraph(this);
			tasks.push_back(std::move(task));
		}

		void Setup(RenderSystem & render_system)
		{
			for (auto& task : tasks)
			{
				task->Setup(render_system);
			}
		}

		void Execute(RenderSystem & render_system, SceneGraph & scene_graph)
		{
			for (auto& task : tasks)
			{
				task->Execute(render_system, scene_graph);
			}
		}

		template<typename T>
		auto GetData()
		{
			for (auto& task : tasks)
			{
				if (typeid(T) == task->data_type_info)
				{
					return static_cast<T*>(task->data);
				}
			}
		}

	private:
		std::vector<std::unique_ptr<BaseTask>> tasks;
		std::vector<std::unique_ptr<ResourceBase>> resources;

	};

} /* fg::wr */
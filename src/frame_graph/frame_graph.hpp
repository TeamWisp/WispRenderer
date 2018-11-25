#pragma once

#include <functional>

#include "render_task.hpp"
#include "../util/thread_pool.hpp"

namespace wr
{

	class FrameGraph
	{
	public:
		FrameGraph() : m_thread_pool(settings::num_frame_graph_threads) {}
		virtual ~FrameGraph();

		FrameGraph(const FrameGraph&) = delete;
		FrameGraph(FrameGraph&&) = default;
		FrameGraph& operator=(const FrameGraph&) = delete;
		FrameGraph& operator=(FrameGraph&&) = default;

		template<typename T>
		void AddTask(std::string const & name, decltype(RenderTask<T>::setup_func_type) setup, decltype(RenderTask<T>::execute_func_type) execute);
		void AddTask(std::unique_ptr<BaseRenderTask> task);
		void Setup(RenderSystem & render_system);
		void Execute(RenderSystem & render_system, SceneGraph & scene_graph);
		void Destroy();

		template<typename T>
		std::vector<T*> GetAllCommandLists() const
		{
			static_assert(std::is_base_of<CommandList, T>::value, "Type must be child of wr::CommandList");

			std::vector<T*> retval;
			for (auto& task : m_tasks)
			{
				retval.push_back(task->GetCommandList<T>(true).first);
			}

			return retval;
		}

		template<typename T>
		auto GetData();

	private:
		std::vector<std::unique_ptr<BaseRenderTask>> m_tasks;
		// Non owning reference to a task in the m_tasks vector.
		std::vector<BaseRenderTask*> m_multi_threaded_tasks;
		// Non owning reference to a task in the m_tasks vector.
		std::vector<BaseRenderTask*> m_single_threaded_tasks;

		util::ThreadPool m_thread_pool;
	};

	//! Used to add a task by creating a new one.
	template<typename T>
	void FrameGraph::AddTask(std::string const & name, decltype(RenderTask<T>::setup_func_type) setup, decltype(RenderTask<T>::execute_func_type) execute)
	{
		m_tasks.push_back(std::make_unique<RenderTask<T>>(this, name, setup, execute));
	}

	//! Used to obtain data from a previously run render task.
	template<typename T>
	auto FrameGraph::GetData()
	{
		for (auto& task : m_tasks)
		{
			if (typeid(T) == task->m_data_type_info)
			{
				return static_cast<RenderTask<T>*>(task.get())->GetData();
			}
		}
	}

} /* wr */
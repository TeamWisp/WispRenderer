#pragma once

#include <functional>
#include "scene_graph.hpp"

namespace wr
{
	namespace fg
	{
		using setup_func_type = std::function<void(RenderSystem&)>;
		using execute_func_type = std::function<void(RenderSystem&, SceneGraph&)>;

		class BaseTask;

		class FrameGraph
		{
		public:
			explicit FrameGraph(wr::RenderSystem& render_system) {}
			virtual ~FrameGraph() = default;

			FrameGraph(const FrameGraph&) = delete;
			FrameGraph(FrameGraph&&) = default;
			FrameGraph& operator=(const FrameGraph&) = delete;
			FrameGraph& operator=(FrameGraph&&) = default;

			std::vector<std::unique_ptr<BaseTask>> tasks;

			template<typename T>
			void AddTask(std::string const & name, setup_func_type setup, execute_func_type execute)
			{
				tasks.push_back(std::make_unique<Task<T>>(name, setup, execute));
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
		};

		class BaseTask
		{
			friend class FrameGraph;
		public:
			BaseTask(const std::type_info& data_type_info, std::string const & name, setup_func_type setup, execute_func_type execute)
				: data_type_info(data_type_info),
				setup(setup), execute(execute), name(name) {}
		private:
			setup_func_type setup;
			execute_func_type execute;

			std::string name;
			bool cull_imune;
			std::size_t ref_count;

			void* data;
			const std::type_info& data_type_info;
		};

		template<typename T>
		class Task : public BaseTask
		{
		public:
			Task(std::string const & name, setup_func_type setup, execute_func_type execute) : BaseTask(typeid(T), name, setup, execute) {}
		};

		class Resource
		{
			std::size_t id;
			std::string name;
			std::size_t ref_count;
		};

	}
}
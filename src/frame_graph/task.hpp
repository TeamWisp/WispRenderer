#pragma once

#include "base_task.hpp"

namespace wr
{
	class RenderSystem;
	class SceneGraph;
} /* wr */

namespace wr::fg
{

	template<typename T>
	class Task : public BaseTask
	{
	public:
		using setup_func_type = std::function<void(RenderSystem&, Task<T>&, T&)>;
		using execute_func_type = std::function<void(RenderSystem&, Task<T>&, SceneGraph&, T&)>;

		Task(FrameGraph* frame_graph, std::string const & name, setup_func_type setup, execute_func_type execute)
			: BaseTask(typeid(T), frame_graph, name), setup(setup), execute(execute) {}

		virtual ~Task() = default;

		virtual void Setup(wr::RenderSystem& render_system) final
		{
			setup(render_system, *this, data);
		}

		virtual void Execute(wr::RenderSystem& render_system, SceneGraph& scene_graph) final
		{
			execute(render_system, *this, scene_graph, data);
		}

	private:
		T data;
		setup_func_type setup;
		execute_func_type execute;
	};

} /* wr::fg */
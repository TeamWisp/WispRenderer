#pragma once

#include "base_render_task.hpp"

namespace wr
{

	class RenderSystem;
	class SceneGraph;

} /* wr */

namespace wr::fg
{

	template<typename T>
	class RenderTask : public BaseRenderTask
	{
	public:
		using setup_func_type = std::function<void(RenderSystem&, RenderTask<T>&, T&)>;
		using execute_func_type = std::function<void(RenderSystem&, RenderTask<T>&, SceneGraph&, T&)>;

		RenderTask(FrameGraph* frame_graph, std::string const & name, setup_func_type setup, execute_func_type execute)
			: BaseRenderTask(typeid(T), frame_graph, name), m_setup(setup), m_execute(execute) {}

		virtual ~RenderTask() = default;

		//! Invokes the bound setup function ptr.
		/*
		  This is done here since I'm unable to call this from the BaseRenderTask since the function pointer type is based on the template T.
		*/
		void Setup(wr::RenderSystem& render_system) final
		{
			m_setup(render_system, *this, m_data);
		}

		//! Invokes the bound execute function ptr.
		/*
		  This is done here since I'm unable to call this from the BaseRenderTask since the function pointer type is based on the template T.
		*/
		void Execute(wr::RenderSystem& render_system, SceneGraph& scene_graph) final
		{
			m_execute(render_system, *this, scene_graph, m_data);
		}

	private:
		//! The struct containing the resources used by the render task.
		T m_data;
		//! The setup function ptr.
		setup_func_type m_setup;
		//! The execute function ptr.
		execute_func_type m_execute;
	};

} /* wr::fg */
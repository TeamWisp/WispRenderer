#pragma once

#include "base_render_task.hpp"
#include "../renderer.hpp"
#include "../d3d12/d3d12_renderer.hpp"

namespace wr
{

	class RenderSystem;
	class SceneGraph;

} /* wr */

namespace wr
{

	template<typename T>
	class RenderTask : public BaseRenderTask
	{
		friend class FrameGraph;
	public:
		struct Data
		{
			RenderTarget* m_render_target;
			RenderTargetProperties m_rt_properties;
			T& m_data;
		};

		using setup_func_type = std::function<void(RenderSystem&, RenderTask<T>&, T&, bool)>;
		using execute_func_type = std::function<void(RenderSystem&, RenderTask<T>&, SceneGraph&, T&)>;
		using destroy_func_type = std::function<void(RenderTask<T>&, T&, bool)>;

		RenderTask(FrameGraph* frame_graph,
			std::string const & name,
			RenderTaskType type,
			bool allow_multithreading,
			RenderTargetProperties rt_properties,
			setup_func_type setup,
			execute_func_type execute,
			destroy_func_type destroy)
			: BaseRenderTask(typeid(T), frame_graph, name, type, allow_multithreading, rt_properties), 
			m_setup(setup),
			m_execute(execute), 
			m_destroy(destroy) {}

		~RenderTask() final
		{
			m_destroy(*this, m_data, false);
		}

		//! Invokes the bound setup function ptr.
		/*
		  This is done here since I'm unable to call this from the BaseRenderTask since the function pointer type is based on the template T.
		*/
		void Setup(wr::RenderSystem& render_system) final
		{


			switch (m_type)
			{
			case RenderTaskType::DIRECT:
				m_cmd_list = render_system.GetDirectCommandList(d3d12::settings::num_back_buffers);
				break;
			case RenderTaskType::COMPUTE:
				m_cmd_list = render_system.GetComputeCommandList(d3d12::settings::num_back_buffers);
				break;
			case RenderTaskType::COPY:
				m_cmd_list = render_system.GetCopyCommandList(d3d12::settings::num_back_buffers);
				break;
			case RenderTaskType::BUNDLE:
				m_cmd_list = render_system.GetBundleCommandList(1);
				break;
			default:
				LOGC("Unknown render task type.");
				break;
			}

			m_render_target = render_system.GetRenderTarget(m_rt_properties);

			//render_system.StartRenderTask(m_cmd_list, { m_render_target, m_rt_properties });

			m_setup(render_system, *this, m_data, false);

			//render_system.StopRenderTask(m_cmd_list, { m_render_target, m_rt_properties });
		}

		//! Invokes the bound execute function ptr.
		/*
		  This is done here since I'm unable to call this from the BaseRenderTask since the function pointer type is based on the template T.
		*/
		void Execute(wr::RenderSystem& render_system, SceneGraph& scene_graph) final
		{
			render_system.StartRenderTask(m_cmd_list, { m_render_target, m_rt_properties });

			m_execute(render_system, *this, scene_graph, m_data);

			render_system.StopRenderTask(m_cmd_list, { m_render_target, m_rt_properties });
		}

		void Resize(RenderSystem& render_system, std::uint32_t width, std::uint32_t height) final
		{
			if constexpr (settings::use_multithreading)
			{
				WaitForCompletion();
			}

			m_destroy(*this, m_data, true);

			if (!m_rt_properties.m_is_render_window)
			{
				render_system.ResizeRenderTarget(&m_render_target, width, height);
			}

			m_setup(render_system, *this, m_data, true);
		}

		Data GetData()
		{
			if constexpr (settings::use_multithreading)
			{
				WaitForCompletion();
			}
			return Data{ m_render_target, m_rt_properties, m_data };
		}

	private:
		//! The struct containing the resources used by the render task.
		T m_data;
		//! The setup function ptr.
		setup_func_type m_setup;
		//! The execute function ptr.
		execute_func_type m_execute;
		//! The destroy function ptr.
		destroy_func_type m_destroy;
	};

} /* wr */
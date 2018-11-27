#pragma once

#include <functional>
#include <string>
#include <array>
#include <optional>
#include <future>

#include "../settings.hpp"
#include "../platform_independend_structs.hpp"

namespace wr
{
	class RenderSystem;
	class SceneGraph;
	class CommandList;
	class RenderTarget;
	class FrameGraph;
} /* wr */

namespace wr
{

	enum class RenderTaskType
	{
		DIRECT,
		BUNDLE,
		COMPUTE,
		COPY
	};

	class BaseRenderTask
	{
		friend class FrameGraph;
	public:
		BaseRenderTask(const std::type_info& data_type_info, FrameGraph* frame_graph, std::string const & name, RenderTaskType type, bool allow_multithreading, RenderTargetProperties rt_properties);
		virtual ~BaseRenderTask();

		BaseRenderTask(const BaseRenderTask&) = delete;
		BaseRenderTask(BaseRenderTask&&) = default;
		BaseRenderTask& operator=(const BaseRenderTask&) = delete;
		BaseRenderTask& operator=(BaseRenderTask&&) = default;

		template<typename T>
		std::pair<T*, RenderTaskType> GetCommandList(bool wait = false)
		{
			static_assert(std::is_base_of<CommandList, T>::value, "Type must be child of wr::CommandList");

			auto n_cmd_list = static_cast<T*>(m_cmd_list);

			if constexpr (settings::use_multithreading)
			{
				if (wait)
				{
					WaitForCompletion();
				}
			}

			return { n_cmd_list, m_type };
		}

		template<typename T>
		T* GetRenderTarget()
		{
			static_assert(std::is_base_of<RenderTarget, T>::value, "Type must be child of wr::RenderTarget");

			return static_cast<T*>(m_render_target);
		}

		void SetFuture(std::future<void>& future);
		void SetFrameGraph(FrameGraph* frame_graph);
		void WaitForCompletion();
		FrameGraph* GetFrameGraph();

		virtual void Setup(wr::RenderSystem&) = 0;
		virtual void Execute(wr::RenderSystem&, SceneGraph&) = 0; // TODO This could be const.
		virtual void Resize(wr::RenderSystem&, std::uint32_t, std::uint32_t) = 0;

	protected:
		FrameGraph* m_frame_graph;
		std::string m_name;
		//! Whether the render task is cullable.
		bool m_cull_immune;
		//! The type info of the data type stored by `RenderTask`.
		const std::type_info& m_data_type_info;
		//! Handle to a cmd_list obtained from the render system.
		CommandList* m_cmd_list;
		//! The Task Render target.
		RenderTarget* m_render_target;
		//! The type of render task
		RenderTaskType m_type;
		//! Render Target Properties
		RenderTargetProperties m_rt_properties;
		//! A pointer to a future used to synchronize the tasks.
		std::future<void> m_future;
		//! Deterimines whether the task should wait or has already waited.
		bool m_new_future;
		//! Is able to use multithreading
		bool m_allow_multithreading;
	};

} /* wr */
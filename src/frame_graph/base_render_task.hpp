#pragma once

#include <functional>
#include <string>

namespace wr
{
	class RenderSystem;
	class SceneGraph;
} /* wr */

namespace wr::fg
{

	class BaseRenderTask
	{
		friend class FrameGraph;
	public:
		BaseRenderTask(const std::type_info& data_type_info, FrameGraph* frame_graph, std::string const & name);
		virtual ~BaseRenderTask() = default;

		BaseRenderTask(const BaseRenderTask&) = delete;
		BaseRenderTask(BaseRenderTask&&) = default;
		BaseRenderTask& operator=(const BaseRenderTask&) = delete;
		BaseRenderTask& operator=(BaseRenderTask&&) = default;

		void SetFrameGraph(FrameGraph* frame_graph);

		virtual void Setup(wr::RenderSystem&) = 0;
		virtual void Execute(wr::RenderSystem&, SceneGraph&) = 0; // TODO This could be const.

	protected:
		FrameGraph* m_frame_graph;
		std::string m_name;
		//! Whether the render task is cullable.
		bool m_cull_immune;
		//! The type info of the data type stored by `RenderTask`.
		const std::type_info& m_data_type_info;
	};

} /* wr::fg */
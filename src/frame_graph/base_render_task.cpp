#include "base_render_task.hpp"

#include "frame_graph.hpp"
#include "../renderer.hpp"

namespace wr
{

	BaseRenderTask::BaseRenderTask(const std::type_info& data_type_info,
		FrameGraph* frame_graph,
		std::string const & name,
		RenderTaskType type,
		bool allow_multithreading,
		RenderTargetProperties rt_properties)
		: m_data_type_info(data_type_info),
		m_frame_graph(frame_graph),
		m_name(name),
		m_cmd_list(nullptr),
		m_type(type),
		m_rt_properties(rt_properties),
		m_new_future(false),
		m_allow_multithreading(allow_multithreading)
	{
	}

	void BaseRenderTask::SetFuture(std::future<void>& future)
	{
		m_future = std::move(future);
		m_new_future = true;
	}

	void BaseRenderTask::WaitForCompletion()
	{
		if (!m_new_future || !m_future.valid()) return;

		m_future.wait();
		m_new_future = false;
	}

	//! Sets the assosiated framegraph
	void BaseRenderTask::SetFrameGraph(FrameGraph* frame_graph)
	{
		m_frame_graph = frame_graph;
	}

	FrameGraph * BaseRenderTask::GetFrameGraph()
	{
		return m_frame_graph;
	}

} /* wr */
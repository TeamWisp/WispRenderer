#include "base_render_task.hpp"

#include "frame_graph.hpp"

namespace wr::fg
{

	BaseRenderTask::BaseRenderTask(const std::type_info& data_type_info, FrameGraph* frame_graph, std::string const & name)
		: m_data_type_info(data_type_info), m_frame_graph(frame_graph), m_name(name)
	{
	}

	//! Sets the assosiated framegraph
	void BaseRenderTask::SetFrameGraph(FrameGraph* frame_graph)
	{
		m_frame_graph = frame_graph;
	}

} /* wr::fg */
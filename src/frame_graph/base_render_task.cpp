#include "base_render_task.hpp"

#include "frame_graph.hpp"
#include "../renderer.hpp"

namespace wr
{

	BaseRenderTask::BaseRenderTask(const std::type_info& data_type_info, FrameGraph* frame_graph, std::string const & name, RenderTaskType type, std::optional<std::pair<unsigned int, unsigned int>> size)
		: m_data_type_info(data_type_info), m_frame_graph(frame_graph), m_name(name), m_cmd_list(nullptr), m_type(type), m_size(size)
	{
	}

	//! Sets the assosiated framegraph
	void BaseRenderTask::SetFrameGraph(FrameGraph* frame_graph)
	{
		m_frame_graph = frame_graph;
	}

} /* wr */
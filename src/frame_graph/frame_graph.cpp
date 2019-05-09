#include "frame_graph.hpp"

using namespace wr;

std::stack<std::uint64_t> FrameGraph::m_free_uids = {};

FrameGraph::FrameGraph(std::size_t num_reserved_tasks)
	: m_render_system(nullptr)
	, m_num_tasks(0)
	, m_thread_pool(new util::ThreadPool(settings::num_frame_graph_threads))
	, m_uid(GetFreeUID())
{
	// lambda to simplify reserving space.
	auto reserve = [num_reserved_tasks](auto v) { v.reserve(num_reserved_tasks); };

	// Reserve space for all vectors.
	reserve(m_setup_funcs);
	reserve(m_execute_funcs);
	reserve(m_destroy_funcs);
	reserve(m_cmd_lists);
	reserve(m_render_targets);
	reserve(m_data);
	reserve(m_data_type_info);
#ifndef FG_MAX_PERFORMANCE
	reserve(m_dependencies);
#endif
	reserve(m_types);
	reserve(m_rt_properties);
	m_settings = decltype(m_settings)(num_reserved_tasks, std::nullopt); // Resizing so I can initialize it with null since this is an optional value.
	m_futures.resize(num_reserved_tasks); // std::thread doesn't allow me to reserve memory for the vector. Hence I'm resizing.
}

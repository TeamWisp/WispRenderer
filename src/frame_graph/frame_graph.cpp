#include "frame_graph.hpp"

using namespace wr;

std::stack<std::uint64_t> FrameGraph::m_free_uids = {};

FrameGraph::FrameGraph(std::size_t num_reserved_tasks)
	: m_uid(GetFreeUID())
{}

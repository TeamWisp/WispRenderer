#include "frame_graph.hpp"

namespace wr
{

	std::uint64_t FrameGraph::m_largest_uid = 0;
	std::stack<std::uint64_t> FrameGraph::m_free_uids = decltype(FrameGraph::m_free_uids)();

} /* wr */
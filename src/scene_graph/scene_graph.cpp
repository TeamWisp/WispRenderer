#include "scene_graph.hpp"

#include <algorithm>

#include "../renderer.hpp"
#include "../util/log.hpp"

namespace wr
{

	SceneGraph::SceneGraph(RenderSystem* render_system)
		: m_render_system(render_system), m_root(std::make_shared<Node>())
	{
	}

	SceneGraph::~SceneGraph()
	{
		RemoveChildren(GetRootNode());
	}

	//! Used to obtain the root node.
	std::shared_ptr<Node> & SceneGraph::GetRootNode()
	{
		return m_root;
	}

	//! Used to obtain the children of a node.
	std::vector<std::shared_ptr<Node>> SceneGraph::GetChildren(std::shared_ptr<Node> const & parent)
	{
		return parent ? parent->m_children : m_root->m_children;
	}

	//! Used to remove the children of a node.
	void SceneGraph::RemoveChildren(std::shared_ptr<Node> const & parent)
	{
		parent->m_children.clear();
	}

	std::shared_ptr<CameraNode> SceneGraph::GetActiveCamera()
	{
		for (auto& camera_node : m_camera_nodes)
		{
			if (camera_node->m_active)
			{
				return camera_node;
			}
		}

		LOGW("Failed to obtain a active camera node.");
		return nullptr;
	}

	void SceneGraph::Init()
	{
		using recursive_func_t = std::function<void(std::shared_ptr<Node>)>;
		recursive_func_t recursive_init = [this, &recursive_init](std::shared_ptr<Node> const & parent)
		{
			for (auto & node : parent->m_children)
			{
				node->Init(m_render_system, node.get());
				recursive_init(node);
			}
		};

		recursive_init(GetRootNode());
	}

} /* wr */
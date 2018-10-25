#include "scene_graph.hpp"

#include <algorithm>

#include "renderer.hpp"

wr::SceneGraph::SceneGraph(std::shared_ptr<RenderSystem> const & render_system)
	: m_render_system(render_system), m_root(std::make_shared<Node>())
{
}

wr::SceneGraph::~SceneGraph()
{
	RemoveChildren(GetRootNode());
}

std::shared_ptr<wr::Node> const & wr::SceneGraph::GetRootNode()
{
	return m_root;
}

std::vector<std::shared_ptr<wr::Node>> wr::SceneGraph::GetChildren(std::shared_ptr<Node> const & parent)
{
	return parent ? parent->m_children : m_root->m_children;
}

void wr::SceneGraph::RemoveChildren(std::shared_ptr<Node> const & parent)
{
	parent->m_children.clear();
}
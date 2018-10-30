#pragma once

#include <vector>
#include <functional>
#include <memory>

#include "../util/defines.hpp"

namespace wr
{

	class RenderSystem;

	struct Node : std::enable_shared_from_this<Node>
	{
		std::shared_ptr<Node> m_parent;
		std::vector<std::shared_ptr<Node>> m_children;

		std::function<void(RenderSystem*, Node*)> Init;
		std::function<void(RenderSystem*, Node*)> Render;
	};

	struct MeshNode : Node
	{
		DECL_SUBNODE(MeshNode)
	};

	struct AnimNode : Node
	{
		DECL_SUBNODE(AnimNode)
	};

	class SceneGraph
	{
	public:
		explicit SceneGraph(std::shared_ptr<RenderSystem> const & render_system);
		~SceneGraph();

		SceneGraph(SceneGraph&&) = delete;
		SceneGraph(SceneGraph const &) = delete;
		SceneGraph& operator=(SceneGraph&&) = delete;
		SceneGraph& operator=(SceneGraph const &) = delete;

		std::shared_ptr<Node> & GetRootNode();
		template<typename T, typename... Args>
		std::shared_ptr<T> CreateChild(std::shared_ptr<Node> const & parent = nullptr, Args... args);
		std::vector<std::shared_ptr<Node>> GetChildren(std::shared_ptr<Node> const & parent = nullptr);
		void RemoveChildren(std::shared_ptr<Node> const & parent);

	private:
		std::shared_ptr<RenderSystem> const & m_render_system;
		//! The root node of the hiararchical tree.
		std::shared_ptr<Node> m_root;
	};

	//! Creates a child into the scene graph
	/*
	  If the parent is a nullptr the child will be created on the root node.
	*/
	template<typename T, typename... Args>
	std::shared_ptr<T> SceneGraph::CreateChild(std::shared_ptr<Node> const & parent, Args... args)
	{
		auto p = parent ? parent : m_root;

		auto new_node = std::make_shared<T>(args...);
		p->m_children.push_back(new_node);
		new_node->m_parent = p;

		return new_node;
	}

} /* wr */
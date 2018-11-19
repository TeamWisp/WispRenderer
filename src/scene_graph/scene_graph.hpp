#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <DirectXMath.h>

#include "../util/defines.hpp"
#include "../resource_pool.hpp"

namespace wr
{
	class RenderSystem;

	struct Node : std::enable_shared_from_this<Node>
	{
		Node() : m_requires_update{ true, true, true }
		{

		}

		std::shared_ptr<Node> m_parent;
		std::vector<std::shared_ptr<Node>> m_children;

		std::function<void(RenderSystem*, Node*)> Init;
		std::function<void(RenderSystem*, Node*)> Update;
		std::function<void(RenderSystem*, Node*)> Render;

		void SignalChange()
		{
			m_requires_update = { true, true, true };
		}

		bool RequiresUpdate(unsigned int frame_idx)
		{
			return m_requires_update[frame_idx];
		}

		std::vector<bool> m_requires_update;
	};

	struct CameraNode;

	class SceneGraph
	{
	public:
		explicit SceneGraph(RenderSystem* render_system);
		~SceneGraph();

		SceneGraph(SceneGraph&&) = delete;
		SceneGraph(SceneGraph const &) = delete;
		SceneGraph& operator=(SceneGraph&&) = delete;
		SceneGraph& operator=(SceneGraph const &) = delete;

		std::shared_ptr<Node> GetRootNode() const;
		template<typename T, typename... Args>
		std::shared_ptr<T> CreateChild(std::shared_ptr<Node> const & parent = nullptr, Args... args);
		std::vector<std::shared_ptr<Node>> GetChildren(std::shared_ptr<Node> const & parent = nullptr);
		void RemoveChildren(std::shared_ptr<Node> const & parent);
		std::shared_ptr<CameraNode> GetActiveCamera();

	private:
		RenderSystem* m_render_system;
		//! The root node of the hiararchical tree.
		std::shared_ptr<Node> m_root;

		std::vector<std::shared_ptr<CameraNode>> m_camera_nodes;
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

		if constexpr (std::is_same<T, CameraNode>::value)
		{
			m_camera_nodes.push_back(new_node);
		}

		return new_node;
	}

} /* wr */
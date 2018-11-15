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
		std::shared_ptr<Node> m_parent;
		std::vector<std::shared_ptr<Node>> m_children;

		std::function<void(RenderSystem*, Node*)> Init;
		std::function<void(RenderSystem*, Node*)> Update;
		std::function<void(RenderSystem*, Node*)> Render;
	};

	struct MeshNode : Node
	{
		DECL_SUBNODE(MeshNode); // TODO: Should be able to dissalow default constructor.
	};

	struct CameraNode : Node
	{
		DECL_SUBNODE(CameraNode);

		CameraNode(float fov, float aspect_ratio)
			: m_active(true),
			m_requires_update{ true, true, true },
			m_pos{ 0, 0, 0},
			m_euler{ 0, 0, 0 },
			m_frustum_near(0.1f),
			m_frustum_far(640.f),
			m_fov(fov),
			m_aspect_ratio(aspect_ratio)
		{
			SUBMODE_CONSTRUCTOR
		}

		void SignalChange()
		{
			m_requires_update = { true, true, true };
		}

		void SetPosition(float x, float y, float z)
		{
			m_pos[0] = x;
			m_pos[1] = y;
			m_pos[2] = z;

			SignalChange();
		}

		void UpdateTemp(unsigned int frame_idx)
		{
			m_requires_update[frame_idx] = false;

			DirectX::XMVECTOR pos{ m_pos[0], m_pos[1], m_pos[2] };

			DirectX::XMVECTOR forward{
				sin(m_euler[1]) * cos(m_euler[0]),
				sin(m_euler[0]),
				cos(m_euler[1]) * cos(m_euler[0]),
			};
			forward = DirectX::XMVector3Normalize(forward);

			auto right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(forward, { world_up[0], world_up[1], world_up[2] }));
			auto up = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(right, forward));	

			m_view = DirectX::XMMatrixLookAtRH(pos, DirectX::XMVectorAdd(pos, forward), up);
			m_projection = DirectX::XMMatrixPerspectiveFovRH(m_fov, m_aspect_ratio, m_frustum_near, m_frustum_far);
		}

		bool RequiresUpdate(unsigned int frame_idx)
		{
			return m_requires_update[frame_idx];
		}

		bool m_active;
		std::vector<bool> m_requires_update;

		float m_pos[3];
		float m_euler[3];

		float m_frustum_near;
		float m_frustum_far;
		float m_fov;
		float m_aspect_ratio;

		DirectX::XMMATRIX m_view;
		DirectX::XMMATRIX m_projection;

		ConstantBufferHandle* m_camera_cb;
	};

	class SceneGraph
	{
	public:
		explicit SceneGraph(RenderSystem* render_system);
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
		std::shared_ptr<CameraNode> GetActiveCamera();

		void Init();

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
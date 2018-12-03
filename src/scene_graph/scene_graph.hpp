#pragma once

#include <bitset>
#include <functional>
#include <memory>
#include <DirectXMath.h>

#include "../util/defines.hpp"
#include "../model_pool.hpp"
#include "../constant_buffer_pool.hpp"
#include "../structured_buffer_pool.hpp"

namespace wr
{
	class RenderSystem;
	struct CommandList;

	struct Node : std::enable_shared_from_this<Node>
	{
		Node()
		{
			SignalChange();
		}

		std::shared_ptr<Node> m_parent;
		std::vector<std::shared_ptr<Node>> m_children;

		void SignalChange()
		{
			m_requires_update[0] = m_requires_update[1] = m_requires_update[2] = true;
		}

		bool RequiresUpdate(unsigned int frame_idx)
		{
			return m_requires_update[frame_idx];
		}

		std::bitset<3> m_requires_update;
	};

	struct CameraNode;
	struct MeshNode;
	struct LightNode;

	enum class LightType : uint32_t
	{
		POINT, DIRECTIONAL, SPOT, FREE /* MAX LighType value; but unused */
	};

	struct Light
	{
		DirectX::XMFLOAT3 pos = { 0, 0, 0 };			//Position in world space for spot & point
		float rad = 5.f;								//Radius for point, height for spot

		DirectX::XMFLOAT3 col = { 1, 1, 1 };			//Color (and strength)
		uint32_t tid = (uint32_t)LightType::FREE;		//Type id; LightType::x

		DirectX::XMFLOAT3 dir = { 0, 0, 1 };			//Direction for spot & directional
		float ang = 40.f / 180.f * 3.1415926535f;		//Angle for spot; in radians
	};

	namespace temp {

		struct ObjectData 
		{

			DirectX::XMMATRIX m_mvp;

			DirectX::XMMATRIX m_vm;
			
			uint32_t m_pad[3];
			uint32_t m_model_id;

		};

		struct MeshBatch_CBData
		{
			std::vector<ObjectData> objects;
		};

		struct MeshBatch
		{
			unsigned int num_instances = 0;
			ConstantBufferHandle* batch_buffer;
			MeshBatch_CBData data;
		};

		using MeshBatches = std::unordered_map<Model*, MeshBatch>;

	}

	class SceneGraph
	{
	public:
		explicit SceneGraph(RenderSystem* render_system);
		~SceneGraph();

		// Impl Functions
		static std::function<void(RenderSystem*, temp::MeshBatches&, CommandList*)> m_render_meshes_func_impl;
		static std::function<void(RenderSystem*, std::vector<std::shared_ptr<MeshNode>>&)> m_init_meshes_func_impl;
		static std::function<void(RenderSystem*, std::vector<std::shared_ptr<CameraNode>>&)> m_init_cameras_func_impl;
		static std::function<void(RenderSystem*, std::vector<std::shared_ptr<LightNode>>&, std::vector<Light>&)> m_init_lights_func_impl;
		static std::function<void(RenderSystem*, std::vector<std::shared_ptr<MeshNode>>&)> m_update_meshes_func_impl;
		static std::function<void(RenderSystem*, std::vector<std::shared_ptr<CameraNode>>&)> m_update_cameras_func_impl;
		static std::function<void(RenderSystem*, std::vector<std::shared_ptr<LightNode>>&, std::vector<Light>&, StructuredBufferHandle*, CommandList*)> m_update_lights_func_impl;

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

		std::vector<std::shared_ptr<LightNode>>& GetLightNodes();

		void Init();
		void Update();
		void Render(CommandList* cmd_list);

		template<typename T>
		void DestroyNode(std::shared_ptr<T> node);

		void Optimize();
		temp::MeshBatches& GetBatches();

		ConstantBufferHandle* GetModelData();
		StructuredBufferHandle* GetLightBuffer();

	private:
		RenderSystem* m_render_system;
		//! The root node of the hiararchical tree.
		std::shared_ptr<Node> m_root;

		temp::MeshBatches m_batches;
		std::vector<Light> m_lights;

		std::shared_ptr<StructuredBufferPool> m_structured_buffer;
		std::shared_ptr<ConstantBufferPool> m_constant_buffer_pool;

		StructuredBufferHandle* m_light_buffer;
		ConstantBufferHandle* m_model_data = nullptr;

		std::vector<std::shared_ptr<CameraNode>> m_camera_nodes;
		std::vector<std::shared_ptr<MeshNode>> m_mesh_nodes;
		std::vector<std::shared_ptr<LightNode>> m_light_nodes;
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
		else if constexpr (std::is_same<T, MeshNode>::value)
		{
			m_mesh_nodes.push_back(new_node);
		}
		else if constexpr (std::is_same<T, LightNode>::value)
		{
			m_light_nodes.push_back(new_node);
		}

		return new_node;
	}

	template<typename T>
	void SceneGraph::DestroyNode(std::shared_ptr<T> node) 
	{
		if constexpr (std::is_same<T, CameraNode>::value)
		{
			for (size_t i = 0, j = m_camera_nodes.size(); i < j; ++i)
			{
				if (m_camera_nodes[i] == node)
				{
					m_camera_nodes.erase(m_camera_nodes.begin() + i);
					break;
				}
			}
		}
		else if constexpr (std::is_same<T, MeshNode>::value)
		{
			for (size_t i = 0, j = m_mesh_nodes.size(); i < j; ++i)
			{
				if (m_mesh_nodes[i] == node)
				{
					m_mesh_nodes.erase(m_mesh_nodes.begin() + i);
					break;
				}
			}
		}
		else if constexpr (std::is_same<T, LightNode>::value)
		{
			for (size_t i = 0, j = m_light_nodes.size(); i < j; ++i)
			{
				if (m_light_nodes[i] == node)
				{
					m_light_nodes.erase(m_light_nodes.begin() + i);
					break;
				}
			}
		}

		node.reset();

	}

} /* wr */
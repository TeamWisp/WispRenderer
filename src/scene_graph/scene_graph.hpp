#pragma once

#include <bitset>
#include <functional>
#include <memory>
#include <DirectXMath.h>

#include "../util/user_literals.hpp"
#include "../util/defines.hpp"
#include "../util/log.hpp"
#include "../model_pool.hpp"
#include "../constant_buffer_pool.hpp"
#include "../structured_buffer_pool.hpp"
#include "../model_pool.hpp"
#include "../util/delegate.hpp"

namespace wr
{
	class RenderSystem;
	struct CameraNode;
	using CommandList = void;

	struct Node : std::enable_shared_from_this<Node>
	{
		Node();

		void SignalChange();
		void SignalUpdate(unsigned int frame_idx);
		bool RequiresUpdate(unsigned int frame_idx);
		
		void SignalTransformChange();
		void SignalTransformUpdate(unsigned int frame_idx);
		bool RequiresTransformUpdate(unsigned int frame_idx);

		//Takes roll, pitch and yaw and converts it to quaternion
		virtual void SetRotation(DirectX::XMVECTOR roll_pitch_yaw);

		//Sets position
		virtual void SetPosition(DirectX::XMVECTOR position);

		//Sets scale
		virtual void SetScale(DirectX::XMVECTOR scale);

		//Position, rotation (roll, pitch, yaw) and scale
		virtual void SetTransform(DirectX::XMVECTOR position, DirectX::XMVECTOR rotation, DirectX::XMVECTOR scale);

		//Update the transform; done automatically when SignalChange is called
		void UpdateTransform();

		std::shared_ptr<Node> m_parent;
		std::vector<std::shared_ptr<Node>> m_children;

		//Translation of mesh node
		DirectX::XMVECTOR m_position = { 0, 0, 0, 1 };
		
		//Rotation as quaternion
		DirectX::XMVECTOR m_rotation;

		//Rotation in radians
		DirectX::XMVECTOR m_rotation_radians = { 0,0,0 };

		//Scale
		DirectX::XMVECTOR m_scale = { 1, 1, 1, 0 };

		//Transformation
		DirectX::XMMATRIX m_local_transform, m_transform;

	private:

		std::bitset<3> m_requires_update;
		std::bitset<3> m_requires_transform_update;

	};

	struct CameraNode;
	struct MeshNode;
	struct LightNode;
	struct SkyboxNode;

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
		float ang = 40._deg;		//Angle for spot; in radians
	};

	namespace temp {

		struct ObjectData {
			DirectX::XMMATRIX m_model;
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
		static util::Delegate<void(RenderSystem*, temp::MeshBatches&, CameraNode* camera, CommandList*)> m_render_meshes_func_impl;
		static util::Delegate<void(RenderSystem*, std::vector<std::shared_ptr<MeshNode>>&)> m_init_meshes_func_impl;
		static util::Delegate<void(RenderSystem*, std::vector<std::shared_ptr<CameraNode>>&)> m_init_cameras_func_impl;
		static util::Delegate<void(RenderSystem*, std::vector<std::shared_ptr<LightNode>>&, std::vector<Light>&)> m_init_lights_func_impl;
		static util::Delegate<void(RenderSystem*, std::vector<std::shared_ptr<MeshNode>>&)> m_update_meshes_func_impl;
		static util::Delegate<void(RenderSystem*, std::vector<std::shared_ptr<CameraNode>>&)> m_update_cameras_func_impl;
		static util::Delegate<void(RenderSystem* render_system, SceneGraph& scene_graph)> m_update_lights_func_impl;
		static util::Delegate<void(RenderSystem* render_system, SceneGraph& scene_graph, std::shared_ptr<Node>&)> m_update_transforms_func_impl;

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
		std::vector<std::shared_ptr<MeshNode>>& GetMeshNodes();
		std::shared_ptr<SkyboxNode> GetCurrentSkybox();

		void Init();
		void Update();
		void Render(CommandList* cmd_list, CameraNode* camera);

		template<typename T>
		void DestroyNode(std::shared_ptr<T> node);

		void Optimize();
		temp::MeshBatches& GetBatches();

		StructuredBufferHandle* GetLightBuffer();
		Light* GetLight(uint32_t offset);			//Returns nullptr when out of bounds

		uint32_t GetCurrentLightSize();

	protected:

		void RegisterLight(std::shared_ptr<LightNode>& light_node);
		void UpdateTransforms(std::shared_ptr<Node>& node);

	private:

		RenderSystem* m_render_system;
		//! The root node of the hiararchical tree.
		std::shared_ptr<Node> m_root;

		temp::MeshBatches m_batches;
		std::vector<Light> m_lights;

		std::shared_ptr<StructuredBufferPool> m_structured_buffer;
		std::shared_ptr<ConstantBufferPool> m_constant_buffer_pool;

		StructuredBufferHandle* m_light_buffer;

		std::vector<std::shared_ptr<CameraNode>> m_camera_nodes;
		std::vector<std::shared_ptr<MeshNode>> m_mesh_nodes;
		std::vector<std::shared_ptr<LightNode>> m_light_nodes;
		std::vector< std::shared_ptr<SkyboxNode>>	m_skybox_nodes;

		uint32_t m_next_light_id = 0;
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

		if constexpr (std::is_base_of<CameraNode, T>::value)
		{
			m_camera_nodes.push_back(new_node);
		}
		else if constexpr (std::is_base_of<MeshNode, T>::value)
		{
			m_mesh_nodes.push_back(new_node);
		}
		else if constexpr (std::is_base_of<LightNode, T>::value)
		{
			RegisterLight(new_node);
		}
		else if constexpr (std::is_same<T, SkyboxNode>::value)
		{
			m_skybox_nodes.push_back(new_node);
		}

		return new_node;
	}

	template<typename T>
	void SceneGraph::DestroyNode(std::shared_ptr<T> node) 
	{
		if constexpr (std::is_base_of<CameraNode, T>::value)
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
		else if constexpr (std::is_base_of<MeshNode, T>::value)
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
		else if constexpr (std::is_base_of<LightNode, T>::value)
		{
			for (size_t i = 0, j = m_light_nodes.size(); i < j; ++i)
			{
				if (m_light_nodes[i] == node)
				{
					//Move everything after this light back one light, so the memory is one filled array

					for (size_t k = i + 1; k < j; ++k)
					{
						--m_light_nodes[k]->m_light;
					}

					//Update light count

					if (m_lights.size() != 0)
					{
						m_lights[0].tid &= 0x3;											//Keep id
						m_lights[0].tid |= uint32_t(m_light_nodes.size() - 1) << 2;		//Set lights
					}

					//Stop tracking the node

					m_light_nodes.erase(m_light_nodes.begin() + i);
					break;
				}
			}
		}

		node.reset();

	}

} /* wr */
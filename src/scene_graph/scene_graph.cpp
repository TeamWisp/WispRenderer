#include "scene_graph.hpp"

#include <algorithm>

#include "../renderer.hpp"
#include "../util/log.hpp"

#include "camera_node.hpp"
#include "mesh_node.hpp"

//TODO: Make platform independent
#include "../d3d12/d3d12_defines.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_resource_pool.hpp"
#include "../d3d12/d3d12_resource_pool_constant_buffer.hpp"

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
	std::shared_ptr<Node> SceneGraph::GetRootNode() const
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

	//! Returns the active camera.
	/*!
		If there are multiple active cameras it will return the first one.
	*/
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

	std::vector<std::shared_ptr<LightNode>>& SceneGraph::GetLightNodes()
	{
		return m_light_nodes;
	}

	//! Initialize the scene graph
	void SceneGraph::Init()
	{
		m_init_meshes_func_impl(m_render_system, m_mesh_nodes);
		m_init_lights_func_impl(m_render_system, m_light_nodes, m_lights);

		// Create constant buffer pool

		constexpr auto model_size = sizeof(temp::ObjectData) * d3d12::settings::num_instances_per_batch;
		constexpr auto cam_size = sizeof(temp::ProjectionView_CBData);

		constexpr auto model_cbs_size = SizeAlign(model_size, 256) * d3d12::settings::num_back_buffers;
		constexpr auto sbo_size = model_cbs_size;

		m_constant_buffer_pool = m_render_system->CreateConstantBufferPool((uint32_t) std::ceil(sbo_size / (1024 * 1024.f)));

		// Initialize cameras

		m_init_cameras_func_impl(m_render_system, m_camera_nodes);

		// Create Light Buffer

		uint64_t light_buffer_stride = sizeof(Light), light_buffer_size = light_buffer_stride * d3d12::settings::num_lights;
		uint64_t light_buffer_aligned_size = SizeAlign(light_buffer_size, 65536) * d3d12::settings::num_back_buffers;

		m_structured_buffer = m_render_system->CreateStructuredBufferPool((size_t) std::ceil(light_buffer_aligned_size / (1024 * 1024.f)));
		m_light_buffer = m_structured_buffer->Create(light_buffer_size, light_buffer_stride, false);

	}

	//! Update the scene graph
	void SceneGraph::Update()
	{
		m_update_meshes_func_impl(m_render_system, m_mesh_nodes);
		m_update_cameras_func_impl(m_render_system, m_camera_nodes);
	}

	//! Render the scene graph
	/*!
		The user is expected to call `Optimize`. If they don't this function will do it manually.
	*/
	void SceneGraph::Render(CommandList* cmd_list)
	{
		bool should_update = m_batches.size() == 0;

		for (auto& elem : m_batches)
		{
			if (elem.second.num_instances == 0)
			{
				should_update = true;
				break;
			}
		}

		if (should_update)
			Optimize();

		m_update_lights_func_impl(m_render_system, m_light_nodes, m_lights, m_light_buffer, cmd_list);
		m_render_meshes_func_impl(m_render_system, m_batches, cmd_list);
	}

	temp::MeshBatches& SceneGraph::GetBatches() 
	{ 
		return m_batches; 
	}

	StructuredBufferHandle* SceneGraph::GetLightBuffer()
	{
		return m_light_buffer;
	}


	void SceneGraph::Optimize() 
	{
		constexpr uint32_t max_size = d3d12::settings::num_instances_per_batch;

		constexpr auto model_size = sizeof(temp::ObjectData) * max_size;

		for (unsigned int i = 0; i < m_mesh_nodes.size(); ++i) {

			auto node = m_mesh_nodes[i];
			auto it = m_batches.find(node->m_model);

			//Insert new if doesn't exist
			if (it == m_batches.end())
			{

				ConstantBufferHandle* object_buffer = m_constant_buffer_pool->Create(model_size);

				auto& batch = m_batches[node->m_model]; 
				batch.batch_buffer = object_buffer;
				batch.data.objects.resize(d3d12::settings::num_instances_per_batch);

				it = m_batches.find(node->m_model);
			}

			//Replace data in buffer
			temp::MeshBatch& batch = it->second;
			unsigned int& offset = batch.num_instances;
			batch.data.objects[offset] = { node->m_transform };
			++offset;

		}

		//Update object data
		for (auto& elem : m_batches)
		{
			temp::MeshBatch& batch = elem.second;
			m_constant_buffer_pool->Update(batch.batch_buffer, model_size, 0, (uint8_t*) batch.data.objects.data());
		}
	}

} /* wr */
#include "scene_graph.hpp"

#include <algorithm>

#include "../renderer.hpp"
#include "../util/log.hpp"

#include "camera_node.hpp"
#include "mesh_node.hpp"
#include "skybox_node.hpp"
#include "light_node.hpp"

//TODO: Make platform independent
#include "../d3d12/d3d12_defines.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"

namespace wr
{

	SceneGraph::SceneGraph(RenderSystem* render_system) :
	    m_render_system(render_system),
		m_root(std::make_shared<Node>()),
		m_light_buffer()
	{
		m_lights.resize(d3d12::settings::num_lights);
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

	std::vector<std::shared_ptr<MeshNode>>& SceneGraph::GetMeshNodes()
	{
		return m_mesh_nodes;
	}

	std::shared_ptr<SkyboxNode> SceneGraph::GetCurrentSkybox()
	{
		if (!m_skybox_nodes.empty())
		{
			return m_skybox_nodes.at(0);
		}
		else
		{
			return nullptr;
		}
	}

	//! Initialize the scene graph
	void SceneGraph::Init()
	{
		m_init_meshes_func_impl(m_render_system, m_mesh_nodes);

		// Create constant buffer pool

		constexpr auto model_size = sizeof(temp::ObjectData) * d3d12::settings::num_instances_per_batch;
		constexpr auto model_cbs_size = SizeAlignTwoPower(model_size, 256) * d3d12::settings::num_back_buffers;

		m_constant_buffer_pool = m_render_system->CreateConstantBufferPool((uint32_t) model_cbs_size*1024);

		// Initialize cameras

		m_init_cameras_func_impl(m_render_system, m_camera_nodes);

		// Create Light Buffer

		std::uint64_t light_count = (std::uint64_t) m_lights.size();
        std::uint64_t light_buffer_stride = sizeof(Light), light_buffer_size = light_buffer_stride * light_count;
        std::uint64_t light_buffer_aligned_size = SizeAlignTwoPower(light_buffer_size, 65536) * d3d12::settings::num_back_buffers;

		m_structured_buffer = m_render_system->CreateStructuredBufferPool((size_t)light_buffer_aligned_size );
		m_light_buffer = m_structured_buffer->Create(light_buffer_size, light_buffer_stride, false);

		//Initialize lights

		m_init_lights_func_impl(m_render_system, m_light_nodes, m_lights);

	}

	//! Update the scene graph
	void SceneGraph::Update()
	{
		m_update_transforms_func_impl(m_render_system, *this, m_root);
		m_update_cameras_func_impl(m_render_system, m_camera_nodes);
		m_update_meshes_func_impl(m_render_system, m_mesh_nodes);
		m_update_lights_func_impl(m_render_system, *this);
	}

	//! Render the scene graph
	/*!
		The user is expected to call `Optimize`. If they don't this function will do it manually.
	*/
	void SceneGraph::Render(CommandList* cmd_list, CameraNode* camera)
	{
		m_render_meshes_func_impl(m_render_system, m_batches, camera, cmd_list);
	}

	temp::MeshBatches& SceneGraph::GetBatches()
	{ 
		return m_batches;
	}

	std::unordered_map<temp::BatchKey, std::vector<temp::ObjectData>, util::PairHash>& SceneGraph::GetGlobalBatches()
	{
		return m_objects;
	}

	StructuredBufferHandle* SceneGraph::GetLightBuffer()
	{
		return m_light_buffer;
	}

	uint32_t SceneGraph::GetCurrentLightSize()
	{
		return m_next_light_id;
	}

	Light* SceneGraph::GetLight(uint32_t offset)
	{
		return offset >= m_next_light_id ? m_lights.data() : m_lights.data() + offset;
	}

	void SceneGraph::RegisterLight(std::shared_ptr<LightNode>& new_node)
	{
		//Allocate a light into the array

		if (m_next_light_id == (uint32_t)m_lights.size())
			LOGE("Couldn't allocate light node; out of memory");

		new_node->m_light = m_lights.data() + m_next_light_id;
		memcpy(new_node->m_light, &new_node->m_temp, sizeof(new_node->m_temp));
		++m_next_light_id;

		//Update light count

		if (!m_lights.empty())
		{
			m_lights[0].tid &= 0x3;											//Keep id
			m_lights[0].tid |= uint32_t(m_light_nodes.size() + 1) << 2;		//Set lights

			if (!m_light_nodes.empty())
			{
				m_light_nodes[0]->SignalChange();
			}
		}

		//Track the node

		m_light_nodes.push_back(new_node);
	}

	void SceneGraph::Optimize() 
	{
		//Update batches

		bool should_update = m_batches.empty();

		for (auto& elem : m_batches)
		{
			if (elem.second.num_instances == 0)
			{
				should_update = true;
				break;
			}
		}

		if (should_update)
		{
			constexpr uint32_t max_size = d3d12::settings::num_instances_per_batch;
			constexpr auto model_size = sizeof(temp::ObjectData) * max_size;

			for (auto& node : m_mesh_nodes) {

				auto mesh_materials_pair = std::make_pair(node->m_model, node->m_materials);

				auto it = m_batches.find(mesh_materials_pair);

				if (node->m_model == nullptr)
				{
					continue;
				}

				auto obj = m_objects.find(mesh_materials_pair);

				//Insert new if doesn't exist
				if (it == m_batches.end())
				{
					ConstantBufferHandle* object_buffer = m_constant_buffer_pool->Create(model_size);

					auto& batch = m_batches[mesh_materials_pair];
					batch.batch_buffer = object_buffer;
					batch.m_materials = node->GetMaterials();
					batch.data.objects.resize(d3d12::settings::num_instances_per_batch);
					
					if (obj == m_objects.end()) {
						m_objects[mesh_materials_pair] = std::vector<temp::ObjectData>(d3d12::settings::num_instances_per_batch);
						obj = m_objects.find(mesh_materials_pair);
					}

					it = m_batches.find(mesh_materials_pair);
				}

				//Replace data in buffer
				temp::MeshBatch& batch = it->second;
				batch.m_materials = node->GetMaterials();

				if (GetActiveCamera()->InView(node) || !d3d12::settings::enable_object_culling) 
				{
					unsigned int& offset = batch.num_instances;
					batch.data.objects[offset] = { node->m_transform, node->m_prev_transform };
					++offset;
				}

				unsigned int& globalOffset = batch.num_global_instances;
				obj->second[globalOffset] = { node->m_transform, node->m_prev_transform };
				++globalOffset;

			}

			std::queue<wr::temp::BatchKey> m_to_remove;

			for (auto& elem : m_batches)
			{
				//Release empty batches
				if (elem.second.num_global_instances == 0)
				{
					m_to_remove.push(elem.first);
					continue;
				}

				//Update object data
				temp::MeshBatch& batch = elem.second;
				m_constant_buffer_pool->Update(batch.batch_buffer, sizeof(temp::ObjectData) * elem.second.num_instances, 0, (uint8_t*)batch.data.objects.data());
			}

			while(!m_to_remove.empty())
			{
				wr::temp::BatchKey &key = m_to_remove.front();

				if (m_batches[key].batch_buffer)
				{
					delete m_batches[key].batch_buffer;
				}

				m_objects.erase(key);
				m_batches.erase(key);
				m_to_remove.pop();
			}

		}
		
	}

} /* wr */

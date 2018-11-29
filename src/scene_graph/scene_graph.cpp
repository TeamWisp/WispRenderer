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

	//! Initialize the scene graph
	void SceneGraph::Init()
	{
		m_init_meshes_func_impl(m_render_system, m_mesh_nodes);
		m_init_cameras_func_impl(m_render_system, m_camera_nodes);

		//TODO: Platform independent (isn't possible with the current pool implementation)

		D3D12RenderSystem* d3d12_render_system = dynamic_cast<D3D12RenderSystem*>(m_render_system);
		auto mesh_details_size = sizeof(CompressedVertex::Details) * d3d12::settings::num_models_per_buffer;

		m_model_data = new D3D12ConstantBufferHandle();
		m_model_data->m_native = d3d12::AllocConstantBuffer(d3d12_render_system->m_cb_heap, mesh_details_size);
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

		m_render_meshes_func_impl(m_render_system, m_batches, cmd_list);
	}

	temp::MeshBatches& SceneGraph::GetBatches() 
	{ 
		return m_batches; 
	}

	D3D12ConstantBufferHandle* SceneGraph::GetModelData()
	{
		return m_model_data;
	}

	void SceneGraph::Optimize() 
	{
		D3D12RenderSystem* d3d12_render_system = dynamic_cast<D3D12RenderSystem*>(m_render_system);
		constexpr uint32_t max_size = d3d12::settings::num_instances_per_batch;

		std::vector<Model*> models;
		models.reserve(m_mesh_nodes.size());

		std::vector<CompressedVertex::Details> mesh_details;
		mesh_details.reserve(m_mesh_nodes.size());

		for (unsigned int i = 0; i < m_mesh_nodes.size(); ++i) {

			auto node = m_mesh_nodes[i];
			auto it = m_batches.find(node->m_model);

			//Insert new if doesn't exist
			if (it == m_batches.end()) {

				auto transform_cb = new D3D12ConstantBufferHandle();
				transform_cb->m_native = d3d12::AllocConstantBuffer(d3d12_render_system->m_cb_heap, sizeof(temp::ObjectData) * d3d12::settings::num_instances_per_batch);

				auto& batch = m_batches[node->m_model]; 
				batch.batchBuffer = transform_cb;
				batch.data.objects.resize(d3d12::settings::num_instances_per_batch);

				it = m_batches.find(node->m_model);
			}

			//Replace data in buffer
			temp::MeshBatch& batch = it->second;
			unsigned int& offset = batch.num_instances;
			unsigned int j = 0;

			if (node->m_model->m_compression_details.m_is_defined)
			{
				auto it = std::find(models.begin(), models.end(), node->m_model);

				if (it == models.end())
				{
					j = models.size();
					models.push_back(node->m_model);
					mesh_details.push_back(node->m_model->m_compression_details);
				}
				else
					j = unsigned int(it - models.begin());

			}

			batch.data.objects[offset] = { node->m_transform, { 0, 0, 0 }, j };
			++offset;

		}

		//Update object data
		for (auto& elem : m_batches)
		{
			temp::MeshBatch& batch = elem.second;
			d3d12::UpdateConstantBuffer(batch.batchBuffer->m_native, d3d12_render_system->GetFrameIdx(), batch.data.objects.data(), sizeof(temp::ObjectData) * d3d12::settings::num_instances_per_batch);
		}

		//Update model data

		auto mesh_details_size = mesh_details.size() * d3d12::settings::num_models_per_buffer;
		d3d12::UpdateConstantBuffer(m_model_data->m_native, d3d12_render_system->GetFrameIdx(), mesh_details.data(), mesh_details_size);

	}

} /* wr */
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

	temp::MeshBatches& SceneGraph::GetBatches() 
	{ 
		return m_batches; 
	}

	void SceneGraph::Optimize() 
	{
		D3D12RenderSystem* d3d12_render_system = dynamic_cast<D3D12RenderSystem*>(m_render_system);
		constexpr uint32_t max_size = d3d12::settings::num_instances_per_batch;

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
			batch.data.objects[offset] = { node->m_transform };
			++offset;

		}

		//Update object data
		for (auto& elem : m_batches)
		{
			temp::MeshBatch& batch = elem.second;
			d3d12::UpdateConstantBuffer(batch.batchBuffer->m_native, d3d12_render_system->GetFrameIdx(), batch.data.objects.data(), sizeof(temp::ObjectData) * d3d12::settings::num_instances_per_batch);
		}
	}

} /* wr */
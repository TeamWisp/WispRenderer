#include "d3d12_renderer.hpp"

#include <iostream>

#include "../util/defines.hpp"
#include "../scene_graph/scene_graph.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../render_tasks/d3d12_deferred_render_task.hpp"

#include "d3d12_resource_pool.hpp"
#include "d3d12_functions.hpp"

LINK_NODE_FUNCTION(wr::D3D12RenderSystem, wr::MeshNode, Init_MeshNode, Render_MeshNode)
LINK_NODE_FUNCTION(wr::D3D12RenderSystem, wr::AnimNode, Init_AnimNode, Render_AnimNode)

namespace wr
{

	void D3D12RenderSystem::Init(std::optional<std::shared_ptr<Window>> const & window)
	{
		m_device = d3d12::CreateDevice();
	}

	std::unique_ptr<Texture> D3D12RenderSystem::Render(std::shared_ptr<SceneGraph> const & scene_graph, fg::FrameGraph& frame_graph)
	{
		using recursive_func_t = std::function<void(std::shared_ptr<Node>)>;
		recursive_func_t recursive_render = [this, &recursive_render](std::shared_ptr<Node> const & parent)
		{
			for (auto & node : parent->m_children)
			{
				node->Render(this, node.get());
				recursive_render(node);
			}
		};

		frame_graph.Execute(*this, *scene_graph.get());

		recursive_render(scene_graph->GetRootNode());

		return std::unique_ptr<Texture>();
	}

	void D3D12RenderSystem::Resize(std::int32_t width, std::int32_t height)
	{
	}

	std::shared_ptr<MaterialPool> D3D12RenderSystem::CreateMaterialPool(std::size_t size_in_mb)
	{
		return std::make_shared<D3D12MaterialPool>(size_in_mb);
	}

	std::shared_ptr<ModelPool> D3D12RenderSystem::CreateModelPool(std::size_t size_in_mb)
	{
		return std::make_shared<D3D12ModelPool>(size_in_mb);
	}

	void D3D12RenderSystem::Init_MeshNode(MeshNode* node)
	{
		std::cout << "initmiesh\n";
	}

	void D3D12RenderSystem::Init_AnimNode(AnimNode* node)
	{
		std::cout << "initanim\n";
	}

	void D3D12RenderSystem::Render_MeshNode(MeshNode* node)
	{
		std::cout << "rendermesh\n";
	}

	void D3D12RenderSystem::Render_AnimNode(AnimNode* node)
	{
		std::cout << "renderanim\n";
	}

} /*  */
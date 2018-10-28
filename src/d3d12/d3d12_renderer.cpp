#include "d3d12_renderer.hpp"

#include <iostream>

#include "../scene_graph/scene_graph.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../render_tasks/d3d12_deferred_render_task.hpp"

#include "d3d12_resource_pool.hpp"
#include "d3d12_functions.hpp"


LINK_NODE_FUNCTION(wr::D3D12RenderSystem, wr::MeshNode, Init_MeshNode, Render_MeshNode)
LINK_NODE_FUNCTION(wr::D3D12RenderSystem, wr::AnimNode, Init_AnimNode, Render_AnimNode)

wr::D3D12RenderSystem::D3D12RenderSystem()
{

}

wr::D3D12RenderSystem::~D3D12RenderSystem()
{

}

void wr::D3D12RenderSystem::Init(std::optional<std::shared_ptr<Window>> const & window)
{
	m_device = d3d12::CreateDevice();
}

std::unique_ptr<wr::Texture> wr::D3D12RenderSystem::Render(std::shared_ptr<SceneGraph> const & scene_graph, fg::FrameGraph& frame_graph)
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

void wr::D3D12RenderSystem::Resize(std::int32_t width, std::int32_t height)
{
}

std::shared_ptr<wr::MaterialPool> wr::D3D12RenderSystem::CreateMaterialPool(std::size_t size_in_mb)
{
	return std::make_shared<D3D12MaterialPool>(size_in_mb);
}

std::shared_ptr<wr::ModelPool> wr::D3D12RenderSystem::CreateModelPool(std::size_t size_in_mb)
{
	return std::make_shared<D3D12ModelPool>(size_in_mb);
}

void wr::D3D12RenderSystem::Init_MeshNode(wr::MeshNode* node)
{
	std::cout << "initmiesh\n";
}

void wr::D3D12RenderSystem::Init_AnimNode(wr::AnimNode* node)
{
	std::cout << "initanim\n";
}

void wr::D3D12RenderSystem::Render_MeshNode(wr::MeshNode* node)
{
	std::cout << "rendermesh\n";
}

void wr::D3D12RenderSystem::Render_AnimNode(wr::AnimNode* node)
{
	std::cout << "renderanim\n";
}

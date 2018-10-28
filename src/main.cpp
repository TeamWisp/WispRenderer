#include "d3d12/d3d12_renderer.hpp"
#include "scene_graph/scene_graph.hpp"
#include "render_tasks/d3d12_deferred_render_task.hpp"
#include "frame_graph/frame_graph.hpp"

#include "d3d12/d3d12_structs.hpp"
#include "d3d12/d3d12_functions.hpp"

int main()
{
	auto render_system = std::make_shared<wr::D3D12RenderSystem>();
	auto scene_graph = std::make_shared<wr::SceneGraph>(render_system);

	scene_graph->CreateChild<wr::MeshNode>();
	auto some_node = scene_graph->CreateChild<wr::MeshNode>();
	scene_graph->CreateChild<wr::MeshNode>(some_node);
	scene_graph->CreateChild<wr::AnimNode>();

	auto resource_manager = render_system->CreateMaterialPool(1);

	wr::fg::FrameGraph frame_graph;
	frame_graph.AddTask(wr::fg::GetDeferredTask());
	frame_graph.Setup(*render_system);

	auto texture = render_system->Render(scene_graph, frame_graph);

	return 0;
}
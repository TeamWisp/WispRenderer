#include "d3d12_renderer.hpp"
#include "scene_graph.hpp"
#include "fg/framegraph.hpp"
#include "d3d12_deferred_render_task.hpp"
#include "frame_graph.hpp"

namespace fg
{
}

int main()
{
	auto render_system = std::make_shared<wr::D3D12RenderSystem>();
	auto scene_graph = std::make_shared<wr::SceneGraph>(render_system);

	scene_graph->CreateChild<wr::MeshNode>();
	auto some_node = scene_graph->CreateChild<wr::MeshNode>();
	scene_graph->CreateChild<wr::MeshNode>(some_node);
	scene_graph->CreateChild<wr::AnimNode>();

	auto resource_manager = render_system->CreateMaterialPool(1);

	fg::framegraph frame_graph(*render_system);
	//wr::CreateDeferredTask(frame_graph);

	frame_graph.compile();

	auto texture = render_system->Render(scene_graph, frame_graph);

	frame_graph.clear();

	struct Task1Data
	{
		bool in_boolean;
		int out_integer;
	};

	wr::fg::FrameGraph fg(*render_system);
	fg.AddTask<Task1Data>(
		"Deferred Render Task",
		[](wr::RenderSystem&) {
	
		},
		[](wr::RenderSystem&, wr::SceneGraph&)
		{

		}
	);
	auto data = fg.GetData<Task1Data>();

	return 0;
}
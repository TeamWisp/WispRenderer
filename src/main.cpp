#include "d3d12/d3d12_renderer.hpp"
#include "window.hpp"
#include "scene_graph/scene_graph.hpp"
#include "render_tasks/d3d12_test_render_task.hpp"
#include "frame_graph/frame_graph.hpp"

#include "d3d12/d3d12_structs.hpp"
#include "d3d12/d3d12_functions.hpp"
#include "util/log.hpp"

int main()
{
	auto render_system = std::make_unique<wr::D3D12RenderSystem>();
	auto window = std::make_unique<wr::Window>(GetModuleHandle(0), "D3D12 Test App", 400, 400);

	render_system->Init(window.get());

	auto scene_graph = std::make_shared<wr::SceneGraph>(render_system.get());

	scene_graph->CreateChild<wr::MeshNode>();
	auto some_node = scene_graph->CreateChild<wr::MeshNode>();
	scene_graph->CreateChild<wr::MeshNode>(some_node);
	scene_graph->CreateChild<wr::AnimNode>();

	auto resource_manager = render_system->CreateMaterialPool(1);

	wr::fg::FrameGraph frame_graph;
	frame_graph.AddTask(wr::fg::GetTestTask());
	frame_graph.Setup(*render_system);

	while (window->IsRunning())
	{
		window->PollEvents();
		auto texture = render_system->Render(scene_graph, frame_graph);
	}

	return 0;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	main();
}
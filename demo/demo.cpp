#include <memory>
#include <algorithm>
#include "wisp.hpp"
#include "render_tasks/d3d12_test_render_task.hpp"
#include "render_tasks/d3d12_imgui_render_task.hpp"
#include "render_tasks/d3d12_deferred_main.hpp"
#include "render_tasks/d3d12_deferred_composition.hpp"
#include "render_tasks/d3d12_deferred_render_target_copy.hpp"
#include "render_tasks/d3d12_raytracing_task.hpp"

#include "engine_interface.hpp"
#include "scene_viknell.hpp"
#include "resources.hpp"
#include "scene_cubes.hpp"

#define SCENE viknell_scene

std::unique_ptr<wr::D3D12RenderSystem> render_system;
std::shared_ptr<wr::SceneGraph> scene_graph;

std::shared_ptr<wr::TexturePool> texture_pool;

int frame_graph = 1;

void RenderEditor()
{
	engine::RenderEngine(render_system.get(), scene_graph.get());
}

int WispEntry()
{
	// ImGui Logging
	util::log_callback::impl = [&](std::string const & str)
	{
		engine::debug_console.AddLog(str.c_str());
	};

	render_system = std::make_unique<wr::D3D12RenderSystem>();
	auto window = std::make_unique<wr::Window>(GetModuleHandleA(nullptr), "D3D12 Test App", 1280, 720);

	window->SetKeyCallback([](int key, int action, int mods)
	{
		if (action == WM_KEYUP && key == 0xC0)
		{
			engine::open_console = !engine::open_console;
			engine::debug_console.EmptyInput();
		}
		if (action == WM_KEYUP && key == VK_F1)
		{
			engine::show_imgui = !engine::show_imgui;
		}
		if (action == WM_KEYUP && key == VK_F2)
		{
			if (frame_graph == 1)
				frame_graph = 2;
			else
				frame_graph = 1;
		}
		if (action == WM_KEYUP && key == VK_F3)
		{
			frame_graph = 3;
		}

	});

	render_system->Init(window.get());

	resources::CreateResources(render_system.get());

	scene_graph = std::make_shared<wr::SceneGraph>(render_system.get());

	SCENE::CreateScene(scene_graph.get(), window.get());

	render_system->InitSceneGraph(*scene_graph.get());

	wr::FrameGraph frame_graph_1;
	if (true)
	{
		frame_graph_1.AddTask(wr::GetDeferredMainTask());
		frame_graph_1.AddTask(wr::GetDeferredCompositionTask());
		frame_graph_1.AddTask(wr::GetRenderTargetCopyTask<wr::DeferredCompositionTaskData>());
	}
	else
	{
		frame_graph_1.AddTask(wr::GetRaytracingTask());
		frame_graph_1.AddTask(wr::GetRenderTargetCopyTask<wr::RaytracingData>());
	}
	frame_graph_1.AddTask(wr::GetImGuiTask(&RenderEditor));
	frame_graph_1.Setup(*render_system);

	wr::FrameGraph frame_graph_2;
	frame_graph_2.AddTask(wr::GetImGuiTask(&RenderEditor));
	frame_graph_2.Setup(*render_system);

	wr::FrameGraph frame_graph_3;

	window->SetResizeCallback([&](std::uint32_t width, std::uint32_t height)
	{
		render_system->WaitForAllPreviousWork();
		frame_graph_1.Resize(*render_system.get(), width, height);
		frame_graph_2.Resize(*render_system.get(), width, height);
		frame_graph_3.Resize(*render_system.get(), width, height);
		render_system->Resize(width, height);
	});

	bool frame_graph_3_created = false;

	while (window->IsRunning())
	{
		window->PollEvents();

		SCENE::UpdateScene();

		std::unique_ptr<wr::TextureHandle> texture;

		switch (frame_graph) {
		case 1:
			texture = render_system->Render(scene_graph, frame_graph_1);
			break;
		case 2:
			texture = render_system->Render(scene_graph, frame_graph_2);
			break;
		case 3:
			if (!frame_graph_3_created)
			{
				frame_graph_3_created = true;
				frame_graph_3.AddTask(wr::GetDeferredMainTask());
				frame_graph_3.AddTask(wr::GetDeferredCompositionTask());
				frame_graph_3.AddTask(wr::GetRenderTargetCopyTask<wr::DeferredCompositionTaskData>());
				frame_graph_3.Setup(*render_system);
			}
			texture = render_system->Render(scene_graph, frame_graph_3);
			break;
		}
	}

	render_system->WaitForAllPreviousWork(); // Make sure GPU is finished before destruction.
	frame_graph_1.Destroy();
	render_system.reset();
	return 0;
}

WISP_ENTRY(WispEntry)

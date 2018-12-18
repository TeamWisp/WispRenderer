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
#include "scene_pbr.hpp"

#include "model_loader_assimp.hpp"

#define SCENE viknell_scene

constexpr bool do_raytracing = true;

std::unique_ptr<wr::D3D12RenderSystem> render_system;
std::shared_ptr<wr::SceneGraph> scene_graph;

std::shared_ptr<wr::TexturePool> texture_pool;

constexpr unsigned int RENDER_TARGET_WIDTH = 1280;
constexpr unsigned int RENDER_TARGET_HEIGHT = 720;

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

	wr::ModelLoader* assimp_model_loader = new wr::AssimpModelLoader();

	render_system->Init(window.get());	

	resources::CreateResources(render_system.get());

	scene_graph = std::make_shared<wr::SceneGraph>(render_system.get());

	SCENE::CreateScene(scene_graph.get());

	render_system->InitSceneGraph(*scene_graph.get());

	wr::FrameGraph frame_graph;
	if (do_raytracing)
	{
		frame_graph.AddTask(wr::GetDeferredMainTask(RENDER_TARGET_WIDTH, RENDER_TARGET_HEIGHT));
		frame_graph.AddTask(wr::GetDeferredCompositionTask(RENDER_TARGET_WIDTH, RENDER_TARGET_HEIGHT));
		frame_graph.AddTask(wr::GetRenderTargetCopyTask<wr::DeferredCompositionTaskData>(RENDER_TARGET_WIDTH, RENDER_TARGET_HEIGHT));
	}
	else
	{
		frame_graph.AddTask(wr::GetRaytracingTask(RENDER_TARGET_WIDTH, RENDER_TARGET_HEIGHT));
		frame_graph.AddTask(wr::GetRenderTargetCopyTask<wr::RaytracingData>(RENDER_TARGET_WIDTH, RENDER_TARGET_HEIGHT));
	}
	frame_graph.AddTask(wr::GetImGuiTask(&RenderEditor, RENDER_TARGET_WIDTH, RENDER_TARGET_HEIGHT));
	frame_graph.Setup(*render_system);

	while (true)
	{
		SCENE::UpdateScene();

		auto texture = render_system->Render(scene_graph, frame_graph);
	}

	delete assimp_model_loader;

	render_system->WaitForAllPreviousWork(); // Make sure GPU is finished before destruction.
	frame_graph.Destroy();
	render_system.reset();
	return 0;
}

WISP_ENTRY(WispEntry)

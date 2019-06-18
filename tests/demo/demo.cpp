#include <memory>
#include <algorithm>
#include <thread>
#include <chrono>
#include <map>
#include <string>
#include <vector>
#include <ctime>
#include <iostream>

#include "wisp.hpp"
#include "version.hpp"
#include "demo_frame_graphs.hpp"
#include "util/file_watcher.hpp"

//Crashpad includes
#include "client/crashpad_client.h"
#include "client/settings.h"
#include "client/crash_report_database.h"

#include "engine_interface.hpp"
#include "physics_engine.hpp"
#include "scene_viknell.hpp"
#include "scene_alien.hpp"
#include "scene_emibl.hpp"
#include "scene_sponza.hpp"

#include "model_loader_assimp.hpp"
#include "model_loader_tinygltf.hpp"
#include "d3d12/d3d12_dynamic_descriptor_heap.hpp"

using DefaultScene = ViknellScene;
//#define ENABLE_PHYSICS

std::unique_ptr<wr::D3D12RenderSystem> render_system;
Scene* current_scene = nullptr;
Scene* new_scene = nullptr;

void RenderEditor(ImTextureID output)
{
	engine::RenderEngine(output, render_system.get(), current_scene, &new_scene);
}

void ShaderDirChangeDetected(std::string const & path, util::FileWatcher::FileStatus status)
{
	auto& registry = wr::PipelineRegistry::Get();
	auto& rt_registry = wr::RTPipelineRegistry::Get();

	if (status == util::FileWatcher::FileStatus::MODIFIED)
	{
		LOG("Change detected in the shader directory. Reloading pipelines and shaders.");

		for (auto it : registry.m_objects)
		{
			registry.RequestReload(it.first);
		}

		for (auto it : rt_registry.m_objects)
		{
			rt_registry.RequestReload(it.first);
		}
	}
}

int WispEntry()
{
	constexpr auto version = wr::GetVersion();
	LOG("Wisp Version {}.{}.{}", version.m_major, version.m_minor, version.m_patch);

	// ImGui Logging
	util::log_callback::impl = [&](std::string const & str)
	{
		engine::debug_console.AddLog(str.c_str());
	};

	render_system = std::make_unique<wr::D3D12RenderSystem>();

	phys::PhysicsEngine phys_engine;

	auto window = std::make_unique<wr::Window>(GetModuleHandleA(nullptr), "D3D12 Test App", 1280, 720);

	window->SetKeyCallback([](int key, int action, int mods)
	{
		current_scene->GetCamera<DebugCamera>()->KeyAction(key, action);

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
			fg_manager::Next();
		}
		if (action == WM_KEYUP && key == VK_F3)
		{
			fg_manager::Prev();
		}
	});

	window->SetMouseCallback([](int key, int action, int mods)
	{
		current_scene->GetCamera<DebugCamera>()->MouseAction(key, action);
	});

	window->SetMouseWheelCallback([](int amount, int action, int mods)
	{
		current_scene->GetCamera<DebugCamera>()->MouseWheel(amount);
	});

	wr::ModelLoader* assimp_model_loader = new wr::AssimpModelLoader();
	wr::ModelLoader* gltf_model_loader = new wr::TinyGLTFModelLoader();

	render_system->Init(window.get());	

	phys_engine.CreatePhysicsWorld();

	current_scene = new DefaultScene();
	current_scene->Init(render_system.get(), window->GetWidth(), window->GetHeight(), &phys_engine);

	fg_manager::Setup(*render_system, &RenderEditor);

	window->SetResizeCallback([&](std::uint32_t width, std::uint32_t height)
	{
		render_system->WaitForAllPreviousWork();
		render_system->Resize(width, height);
		current_scene->GetCamera<wr::CameraNode>()->SetAspectRatio((float)width / (float)height);
		current_scene->GetCamera<wr::CameraNode>()->SetOrthographicResolution(width, height);
		fg_manager::Resize(*render_system, width, height);
	});

	auto file_watcher = new util::FileWatcher("resources/shaders", std::chrono::milliseconds(100));
	file_watcher->StartAsync(&ShaderDirChangeDetected);

	window->SetRenderLoop([&]() {
		// Find delta
		float delta = ImGui::GetIO().DeltaTime;
		bool capture_frame = engine::recorder.ShouldCaptureAndIncrement(delta);
		if (capture_frame)
		{
			fg_manager::Get()->SaveTaskToDisc<wr::PostProcessingData>(engine::recorder.GetNextFilename(".tga"), 0);
		}

		if (new_scene && new_scene != current_scene)
		{
			delete current_scene;
			current_scene = new_scene;
			current_scene->Init(render_system.get(), window->GetWidth(), window->GetHeight(), &phys_engine);
			fg_manager::Get()->SetShouldExecute<wr::EquirectToCubemapTaskData>(true);
			fg_manager::Get()->SetShouldExecute<wr::CubemapConvolutionTaskData>(true);
		}

		current_scene->Update(delta);

#ifdef ENABLE_PHYSICS
		phys_engine.UpdateSim(delta, *scene_graph.get());
#endif

		auto texture = render_system->Render(*current_scene->GetSceneGraph(), *fg_manager::Get());

	});

	window->StartRenderLoop();

	delete assimp_model_loader;
	delete gltf_model_loader;

	render_system->WaitForAllPreviousWork(); // Make sure GPU is finished before destruction.

	delete current_scene;

	fg_manager::Destroy();
	render_system.reset();

	return 0;
}

WISP_ENTRY(WispEntry)

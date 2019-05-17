#include <memory>
#include <algorithm>
#include <thread>
#include <chrono>
#include <map>
#include <string>
#include <vector>

#include "wisp.hpp"
#include "demo_frame_graphs.hpp"
#include "util/file_watcher.hpp"

//Crashpad includes
#include "client/crashpad_client.h"
#include "client/settings.h"
#include "client/crash_report_database.h"

#include "engine_interface.hpp"
#include "scene_viknell.hpp"
#include "scene_emibl.hpp"
#include "scene_spheres.hpp"
#include "scene_sun_temple.hpp"

#include "model_loader_assimp.hpp"
#include "d3d12/d3d12_dynamic_descriptor_heap.hpp"

#define SCENE sun_temple_scene

std::unique_ptr<wr::D3D12RenderSystem> render_system;
std::shared_ptr<wr::SceneGraph> scene_graph;

void RenderEditor(ImTextureID output)
{
	engine::RenderEngine(output, render_system.get(), scene_graph.get());
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

void startCrashpad() 
{
	// Cache directory that will store crashpad information and minidumps
	base::FilePath database(L"CrashPadDB");
	// Path to the out-of-process handler executable
	base::FilePath handler(L"deps/crashpad/out/Release/crashpad_handler.exe");
	// URL used to submit minidumps to
	std::string url("https://sentry.io/api/1455776/minidump/?sentry_key=7735122410eb4b6bbaec9f1f6fd43aff");
	// Optional annotations passed via --annotations to the handler
	std::map<std::string, std::string> annotations;
	// Optional arguments to pass to the handler
	std::vector<std::string> arguments;

	std::unique_ptr<crashpad::CrashReportDatabase> db =
		crashpad::CrashReportDatabase::Initialize(database);

	if (db != nullptr && db->GetSettings() != nullptr) 
	{
		db->GetSettings()->SetUploadsEnabled(true);
	}

	arguments.push_back("--no-rate-limit");

	crashpad::CrashpadClient client;
	bool success = client.StartHandler(
		handler,
		database,
		database,
		url,
		annotations,
		arguments,
		/* restartable */ true,
		/* asynchronous_start */ true
	);

	if (success) 
	{
		success = client.WaitForHandlerStart(INFINITE);
	}
}


int WispEntry()
{
	// ImGui Logging
	util::log_callback::impl = [&](std::string const & str)
	{
		engine::debug_console.AddLog(str.c_str());
	};
	startCrashpad();

	render_system = std::make_unique<wr::D3D12RenderSystem>();
	auto window = std::make_unique<wr::Window>(GetModuleHandleA(nullptr), "D3D12 Test App", 1280, 720);

	window->SetKeyCallback([](int key, int action, int mods)
	{
		SCENE::camera->KeyAction(key, action);

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
		if (action == WM_KEYUP && key == VK_F4)
		{
			SCENE::resources::model_pool->Defragment();
		}
		if (action == WM_KEYUP && key == VK_F5)
		{
			SCENE::resources::model_pool->ShrinkToFit();
		}
	});

	window->SetMouseCallback([](int key, int action, int mods)
	{
		SCENE::camera->MouseAction(key, action);
	});

	window->SetMouseWheelCallback([](int amount, int action, int mods)
	{
		SCENE::camera->MouseWheel(amount);
	});

	wr::ModelLoader* assimp_model_loader = new wr::AssimpModelLoader();

	render_system->Init(window.get());	

	SCENE::resources::CreateResources(render_system.get());

	scene_graph = std::make_shared<wr::SceneGraph>(render_system.get());

	SCENE::CreateScene(scene_graph.get(), window.get());

	render_system->InitSceneGraph(*scene_graph.get());

	fg_manager::Setup(*render_system, &RenderEditor);

	window->SetResizeCallback([&](std::uint32_t width, std::uint32_t height)
	{
		render_system->WaitForAllPreviousWork();
		render_system->Resize(width, height);
		SCENE::camera->SetAspectRatio((float)width / (float)height);
		fg_manager::Resize(*render_system, width, height);
	});

	auto file_watcher = new util::FileWatcher("resources/shaders", std::chrono::milliseconds(100));
	file_watcher->StartAsync(&ShaderDirChangeDetected);

	window->SetRenderLoop([&]() {
		SCENE::UpdateScene();

		auto texture = render_system->Render(scene_graph, *fg_manager::Get());
	});

	window->StartRenderLoop();

	delete assimp_model_loader;

	render_system->WaitForAllPreviousWork(); // Make sure GPU is finished before destruction.

	SCENE::resources::ReleaseResources();

	fg_manager::Destroy();
	render_system.reset();
	return 0;
}

WISP_ENTRY(WispEntry)

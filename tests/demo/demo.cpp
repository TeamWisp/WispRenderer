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
#include "demo_frame_graphs.hpp"
#include "util/file_watcher.hpp"

//Crashpad includes
#include "client/crashpad_client.h"
#include "client/settings.h"
#include "client/crash_report_database.h"

#include "engine_interface.hpp"
#include "physics_engine.hpp"
#include "scene_viknell.hpp"
#include "scene_emibl.hpp"
#include "scene_spheres.hpp"
#include "scene_sun_temple.hpp"

#include "model_loader_assimp.hpp"
#include "model_loader_tinygltf.hpp"
#include "d3d12/d3d12_dynamic_descriptor_heap.hpp"

#define SCENE viknell_scene

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
	std::string url;
	url = "https://WispRenderer.bugsplat.com/post/bp/crash/postBP.php";
	// Optional annotations passed via --annotations to the handler
	std::map<std::string, std::string> annotations;
	annotations["format"] = "minidump";			// Crashpad setting to save crash as a minidump
	annotations["prod"] = "WispRenderer";	    // BugSplat appName
	annotations["ver"] = "1.2.0";				// BugSplat appVersion

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
	
#ifndef _DEBUG //prevents log spam for developers
	std::time_t current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	auto local_time = std::localtime(&current_time);

	std::stringstream ss;
	ss << "log-" << local_time->tm_mday << "-" << (local_time->tm_mon + 1) << "-" << (local_time->tm_year + 1900);
	std::string log_file_name("WispDemo.log");
	util::log_file_handler = new wr::LogfileHandler(std::filesystem::path(ss.str()), log_file_name);
#endif // _DEBUG //prevents log spam for developers

	startCrashpad();

	render_system = std::make_unique<wr::D3D12RenderSystem>();

	phys::PhysicsEngine phys_engine;

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
	wr::ModelLoader* gltf_model_loader = new wr::TinyGLTFModelLoader();

	render_system->Init(window.get());	

	SCENE::resources::CreateResources(render_system.get());

	phys_engine.CreatePhysicsWorld();

	scene_graph = std::make_shared<wr::SceneGraph>(render_system.get());

	SCENE::CreateScene(scene_graph.get(), window.get(), phys_engine);

	render_system->InitSceneGraph(*scene_graph.get());

	fg_manager::Setup(*render_system, &RenderEditor);

	window->SetResizeCallback([&](std::uint32_t width, std::uint32_t height)
	{
		render_system->WaitForAllPreviousWork();
		render_system->Resize(width, height);
		SCENE::camera->SetAspectRatio((float)width / (float)height);
		SCENE::camera->SetOrthographicResolution(width, height);
		fg_manager::Resize(*render_system, width, height);
	});

	auto file_watcher = new util::FileWatcher("resources/shaders", std::chrono::milliseconds(100));
	file_watcher->StartAsync(&ShaderDirChangeDetected);

	window->SetRenderLoop([&]() {
		SCENE::UpdateScene(scene_graph.get());

		phys_engine.UpdateSim(ImGui::GetIO().DeltaTime, *scene_graph.get());

		auto texture = render_system->Render(*scene_graph, *fg_manager::Get());
	});

	window->StartRenderLoop();

	delete assimp_model_loader;
	delete gltf_model_loader;

	render_system->WaitForAllPreviousWork(); // Make sure GPU is finished before destruction.

	SCENE::resources::ReleaseResources();

	fg_manager::Destroy();
	render_system.reset();

#ifndef _DEBUG //cleanup
	delete util::log_file_handler;
#endif 

	return 0;
}

WISP_ENTRY(WispEntry)

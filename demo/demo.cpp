#include <memory>
#include <algorithm>
#include <thread>
#include <chrono>

#include "wisp.hpp"
#include "demo_frame_graphs.hpp"

#include "engine_interface.hpp"
#include "scene_viknell.hpp"
#include "resources.hpp"
#include "scene_cubes.hpp"

#include "model_loader_assimp.hpp"

#define SCENE viknell_scene

std::unique_ptr<wr::D3D12RenderSystem> render_system;
std::shared_ptr<wr::SceneGraph> scene_graph;

std::shared_ptr<wr::TexturePool> texture_pool;

void RenderEditor()
{
	engine::RenderEngine(render_system.get(), scene_graph.get());
}

void SetupShaderDirWatcher()
{
	auto handle = FindFirstChangeNotificationA("resources/shaders/", false,
		FILE_NOTIFY_CHANGE_FILE_NAME |
		FILE_NOTIFY_CHANGE_DIR_NAME |
		FILE_NOTIFY_CHANGE_ATTRIBUTES |
		FILE_NOTIFY_CHANGE_LAST_WRITE |
		FILE_NOTIFY_CHANGE_SECURITY
	);

	auto thread = std::thread([handle]()
	{
		while (true)
		{
			auto wait_status = WaitForSingleObject(handle, INFINITE);
			auto& registry = wr::PipelineRegistry::Get();
			auto& rt_registry = wr::RTPipelineRegistry::Get();

			switch (wait_status)
			{
			case WAIT_OBJECT_0:
				LOG("Change detected in the shader directory. Reloading pipelines and shaders.");

				for (auto it : registry.m_objects)
				{
					registry.RequestReload(it.first);
				}

				for (auto it : rt_registry.m_objects)
				{
					// rt_registry.RequestReload(it.first);
				}

				if (FindNextChangeNotification(handle) == FALSE)
				{
					LOGW("FindNextChangeNotification function failed.");
				}

				using namespace std::chrono_literals;
				std::this_thread::sleep_for(1s);

				break;
			default:
				LOGW("Unhandled wait status.");
				break;
			}
		}
	});

	thread.detach();
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
	});

	window->SetMouseCallback([](int key, int action, int mods)
	{
		SCENE::camera->MouseAction(key, action);
	});

	wr::ModelLoader* assimp_model_loader = new wr::AssimpModelLoader();

	render_system->Init(window.get());	

	resources::CreateResources(render_system.get());

	scene_graph = std::make_shared<wr::SceneGraph>(render_system.get());

	SCENE::CreateScene(scene_graph.get(), window.get());

	render_system->InitSceneGraph(*scene_graph.get());

	fg_manager::Setup(*render_system.get(), &RenderEditor);

	window->SetResizeCallback([&](std::uint32_t width, std::uint32_t height)
	{
		render_system->WaitForAllPreviousWork();
		render_system->Resize(width, height);
		viknell_scene::camera->SetAspectRatio((float)width / (float)height);
		fg_manager::Get()->Resize(*render_system.get(), width, height);
	});

	SetupShaderDirWatcher();

	while (window->IsRunning())
	{
		window->PollEvents();

		SCENE::UpdateScene();

		auto texture = render_system->Render(scene_graph, *fg_manager::Get());
	}

	delete assimp_model_loader;

	render_system->WaitForAllPreviousWork(); // Make sure GPU is finished before destruction.
	fg_manager::Destroy();
	render_system.reset();
	return 0;
}

WISP_ENTRY(WispEntry)

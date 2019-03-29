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
#include "scene_emibl.hpp"
#include "scene_spheres.hpp"

#include "model_loader_assimp.hpp"
#include "d3d12/d3d12_dynamic_descriptor_heap.hpp"

#define SCENE emibl_scene

std::unique_ptr<wr::D3D12RenderSystem> render_system;
std::shared_ptr<wr::SceneGraph> scene_graph;

void RenderEditor(ImTextureID output)
{
	engine::RenderEngine(output, render_system.get(), scene_graph.get());
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
					rt_registry.RequestReload(it.first);
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
		if (action == WM_KEYUP && key == VK_F3)
		{
			fg_manager::Prev();
		}
		if (action == WM_KEYUP && key == VK_F4)
		{
			resources::model_pool->Defragment();
		}
		if (action == WM_KEYUP && key == VK_F5)
		{
			resources::model_pool->ShrinkToFit();
		}
		if (action == WM_KEYUP && key == VK_F6)
		{
			auto model = wr::ModelLoader::m_registered_model_loaders[0]->Load("resources/models/material_ball.fbx");

			std::vector<wr::VertexColor> vertices(model->m_meshes[0]->m_positions.size());
			std::vector<std::uint32_t> indices(model->m_meshes[0]->m_indices.size());

			for (unsigned int j = 0; j < model->m_meshes[0]->m_positions.size(); ++j)
			{
				wr::VertexColor &vertex = vertices[j];

				memcpy(vertex.m_pos, &model->m_meshes[0]->m_positions[j], sizeof(vertex.m_pos));
				memcpy(vertex.m_normal, &model->m_meshes[0]->m_normals[j], sizeof(vertex.m_normal));
				memcpy(vertex.m_tangent, &model->m_meshes[0]->m_tangents[j], sizeof(vertex.m_tangent));
				memcpy(vertex.m_bitangent, &model->m_meshes[0]->m_bitangents[j], sizeof(vertex.m_bitangent));
				memcpy(vertex.m_uv, &model->m_meshes[0]->m_uvw[j], sizeof(vertex.m_uv));
				memcpy(vertex.m_color, &model->m_meshes[0]->m_colors[j], sizeof(vertex.m_color));

			}

			memcpy(indices.data(), model->m_meshes[0]->m_indices.data(), indices.size() * sizeof(std::uint32_t));

			resources::model_pool->EditMesh<wr::VertexColor, std::uint32_t>(resources::test_model->m_meshes[0].first, vertices, indices);

			wr::ModelLoader::m_registered_model_loaders[0]->DeleteModel(model);
		}
		if (action == WM_KEYUP && key == VK_F7)
		{
			auto model = wr::ModelLoader::m_registered_model_loaders[0]->Load("resources/models/xbot.fbx");

			std::vector<wr::VertexColor> vertices(model->m_meshes[0]->m_positions.size());
			std::vector<std::uint32_t> indices(model->m_meshes[0]->m_indices.size());

			for (unsigned int j = 0; j < model->m_meshes[0]->m_positions.size(); ++j)
			{
				wr::VertexColor &vertex = vertices[j];

				memcpy(vertex.m_pos, &model->m_meshes[0]->m_positions[j], sizeof(vertex.m_pos));
				memcpy(vertex.m_normal, &model->m_meshes[0]->m_normals[j], sizeof(vertex.m_normal));
				memcpy(vertex.m_tangent, &model->m_meshes[0]->m_tangents[j], sizeof(vertex.m_tangent));
				memcpy(vertex.m_bitangent, &model->m_meshes[0]->m_bitangents[j], sizeof(vertex.m_bitangent));
				memcpy(vertex.m_uv, &model->m_meshes[0]->m_uvw[j], sizeof(vertex.m_uv));
				memcpy(vertex.m_color, &model->m_meshes[0]->m_colors[j], sizeof(vertex.m_color));

			}

			memcpy(indices.data(), model->m_meshes[0]->m_indices.data(), indices.size() * sizeof(std::uint32_t));

			resources::model_pool->EditMesh<wr::VertexColor, std::uint32_t>(resources::test_model->m_meshes[0].first, vertices, indices);

			wr::ModelLoader::m_registered_model_loaders[0]->DeleteModel(model);
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

	resources::CreateResources(render_system.get());

	scene_graph = std::make_shared<wr::SceneGraph>(render_system.get());

	SCENE::CreateScene(scene_graph.get(), window.get());

	render_system->InitSceneGraph(*scene_graph.get());

	//Are we on fallback or native
	fg_manager::is_fallback = render_system->IsFallback();

	fg_manager::Setup(*render_system.get(), &RenderEditor);

	window->SetResizeCallback([&](std::uint32_t width, std::uint32_t height)
	{
		render_system->WaitForAllPreviousWork();
		render_system->Resize(width, height);
		SCENE::camera->SetAspectRatio((float)width / (float)height);
		fg_manager::Resize(*render_system.get(), width, height);
	});

	SetupShaderDirWatcher();

	while (window->IsRunning())
	{
		window->PollEvents();

		SCENE::UpdateScene();

		auto texture = render_system->Render(scene_graph, *fg_manager::Get());

		// Example usage of the render function output:
		if (texture.pixel_data != std::nullopt)
		{
			// Use pixel data here
		}

		if (texture.depth_data != std::nullopt)
		{
			// Use depth data here
		}
	}

	delete assimp_model_loader;

	render_system->WaitForAllPreviousWork(); // Make sure GPU is finished before destruction.

	resources::ReleaseResources();

	fg_manager::Destroy();
	render_system.reset();
	return 0;
}

WISP_ENTRY(WispEntry)

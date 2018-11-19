#include <memory>

#include "wisp.hpp"
#include "render_tasks/d3d12_test_render_task.hpp"

void WispEntry()
{
	auto render_system = std::make_unique<wr::D3D12RenderSystem>();
	auto window = std::make_unique<wr::Window>(GetModuleHandle(nullptr), "D3D12 Test App", 400, 400);

	render_system->Init(window.get());

	// Load custom model
	auto model_pool = render_system->CreateModelPool(1);
	wr::Model* model;
	{
		wr::MeshData<wr::Vertex> mesh;
		static const constexpr float size = 0.5f;
		mesh.m_indices = std::nullopt;
		mesh.m_vertices = {
			{ -size, -size, 0.f },
			{ size, -size, 0.f },
			{ -size, size, 0.f },
			{ size, size, 0.f },
		};

		model = model_pool->LoadCustom<wr::Vertex>({ mesh });
	}

	auto scene_graph = std::make_shared<wr::SceneGraph>(render_system.get());

	auto mesh_node = scene_graph->CreateChild<wr::MeshNode>(nullptr, model);

	auto camera = scene_graph->CreateChild<wr::CameraNode>(nullptr, 1.74f, (float)window->GetWidth() / (float)window->GetHeight());
	camera->SetPosition(0, 0, -5);

	render_system->InitSceneGraph(*scene_graph.get());

	wr::FrameGraph frame_graph;
	frame_graph.AddTask(wr::GetTestTask());
	frame_graph.Setup(*render_system);

	while (window->IsRunning())
	{
		window->PollEvents();
		auto texture = render_system->Render(scene_graph, frame_graph);
	}
}

WISP_ENTRY(WispEntry)
#include <memory>

#include "wisp.hpp"
#include "render_tasks/d3d12_test_render_task.hpp"
#include "render_tasks/d3d12_imgui_render_task.hpp"
#include "imgui_tools.hpp"

bool main_menu = true;
bool open0 = true;
bool open1 = true;
bool open2 = true;
char message_buffer[600];

std::unique_ptr<wr::D3D12RenderSystem> render_system;

void RenderEditor()
{
	if (main_menu && ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit", "ALT+F4")) std::exit(0);
			ImGui::MenuItem("Theme", nullptr, &open0);
			ImGui::MenuItem("ImGui Details", nullptr, &open1);
			ImGui::MenuItem("Logging Example", nullptr, &open2);
			ImGui::EndMenu();
		}
		wr::imgui::menu::Registries();
		ImGui::EndMainMenuBar();
	}

	ImGui::DockSpaceOverViewport(main_menu, nullptr, ImGuiDockNodeFlags_PassthruDockspace);

	auto& io = ImGui::GetIO();

	// Create dockable background
	if (open0)
	{
		ImGui::Begin("Theme", &open0);
		if (ImGui::Button("Cherry")) ImGui::StyleColorsCherry();
		if (ImGui::Button("Unreal Engine")) ImGui::StyleColorsUE();
		if (ImGui::Button("Light Green")) ImGui::StyleColorsLightGreen();
		if (ImGui::Button("Light")) ImGui::StyleColorsLight();
		if (ImGui::Button("Dark")) ImGui::StyleColorsDark();
		if (ImGui::Button("Dark2")) ImGui::StyleColorsDarkCodz1();
		ImGui::End();
	}

	if (open1)
	{
		ImGui::Begin("ImGui Details", &open1);
		ImGui::Text("Mouse Pos: (%f, %f)", io.MousePos.x, io.MousePos.y);
		ImGui::Text("Framerate: %f", io.Framerate);
		ImGui::Text("Delta: %f", io.DeltaTime);
		ImGui::Text("Delta: {%f, %f}", io.DisplaySize.x, io.DisplaySize.y);
		ImGui::End();
	}

	if (open2)
	{
		ImGui::Begin("Logging Example", &open2);
		ImGui::InputText("Message", message_buffer, 600);
		if (ImGui::Button("LOG (Message)")) LOG(message_buffer);
		if (ImGui::Button("LOGW (Warning)")) LOGW(message_buffer);
		if (ImGui::Button("LOGE (Error)")) LOGE(message_buffer);
		if (ImGui::Button("LOGC (Critical)")) LOGC(message_buffer);
		ImGui::End();
	}

	wr::imgui::window::ShaderRegistry();
	wr::imgui::window::PipelineRegistry();
	wr::imgui::window::RootSignatureRegistry();
	wr::imgui::window::D3D12HardwareInfo(*render_system.get());
}

void WispEntry()
{
	render_system = std::make_unique<wr::D3D12RenderSystem>();
	auto window = std::make_unique<wr::Window>(GetModuleHandleA(nullptr), "D3D12 Test App", 1280, 720);

	render_system->Init(window.get());

	// Load custom model
	auto model_pool = render_system->CreateModelPool(1, 1);
	wr::Model* model;
	{
		wr::MeshData<wr::Vertex> mesh;
		static const constexpr float size = 0.5f;
		mesh.m_indices = std::nullopt;
		mesh.m_vertices = {
			{ -size, -size, 0, 0, 0, 0, 1, 0 },
			{ size, -size, 0, 1, 0, 0, 1, 0 },
			{ -size, size, 0, 0, 1, 0, 1, 0 },
			{ size, size, 0, 1, 1, 0, 1, 0 },
		};

		model = model_pool->LoadCustom<wr::Vertex>({ mesh });
	}

	auto scene_graph = std::make_shared<wr::SceneGraph>(render_system.get());

	auto mesh_node = scene_graph->CreateChild<wr::MeshNode>(nullptr, model);
	auto mesh_node_2 = scene_graph->CreateChild<wr::MeshNode>(nullptr, model);
	auto camera = scene_graph->CreateChild<wr::CameraNode>(nullptr, 1.74f, (float)window->GetWidth() / (float)window->GetHeight());

	mesh_node_2->SetPosition({ -2, -2, 5 });
	camera->SetPosition(0, 0, -5);

	render_system->InitSceneGraph(*scene_graph.get());

	wr::FrameGraph frame_graph;
	frame_graph.AddTask(wr::GetTestTask());
	frame_graph.AddTask(wr::GetImGuiTask(&RenderEditor));
	frame_graph.Setup(*render_system);

	while (window->IsRunning())
	{
		window->PollEvents();
		auto texture = render_system->Render(scene_graph, frame_graph);
	}

	render_system->WaitForAllPreviousWork(); // Make sure GPU is finished before destruction.
	frame_graph.Destroy();
	render_system.reset();
}

WISP_ENTRY(WispEntry)

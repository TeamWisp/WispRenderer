#pragma once

#include "wisp.hpp"
#include "window.hpp"
#include "imgui_tools.hpp"
#include "scene_graph/scene_graph.hpp"
#include "d3d12/d3d12_renderer.hpp"
#include "imgui/ImGuizmo.h"

namespace engine
{

	static bool main_menu = true;
	static bool open0 = true;
	static bool open1 = true;
	static bool open2 = true;
	static bool open_console = false;
	static bool show_imgui = false;
	static bool fullscreen = false;
	static char message_buffer[600];

	static wr::imgui::special::DebugConsole debug_console;

	void RenderEngine(wr::D3D12RenderSystem* render_system, wr::SceneGraph* sg)
	{
		debug_console.Draw("Console", &open_console);

		if (!show_imgui) return;

		if (main_menu && ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Exit", "ALT+F4")) std::exit(0);
				if (ImGui::MenuItem("Hide ImGui", "F1")) show_imgui = false;
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Window"))
			{
				wr::imgui::menu::Registries();
				ImGui::Separator();
				ImGui::MenuItem("Theme", nullptr, &open0);
				ImGui::MenuItem("ImGui Details", nullptr, &open1);
				ImGui::MenuItem("Logging Example", nullptr, &open2);
				ImGui::EndMenu();
			}

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
			ImGui::Text("Mouse Pos: (%0.f, %0.f)", io.MousePos.x, io.MousePos.y);
			ImGui::Text("Framerate: %.0f", io.Framerate);
			ImGui::Text("Delta: %f", io.DeltaTime);
			ImGui::Text("Display Size: (%.0f, %.0f)", io.DisplaySize.x, io.DisplaySize.y);
			ImGui::End();
		}

		if (open2)
		{
			ImGui::Begin("Camera Settings", &open2);
			
			auto pos = sg->GetActiveCamera()->m_position;
			ImGui::DragFloat3("Position", pos.m128_f32, 0.5f);
			sg->GetActiveCamera()->SetPosition(pos);

			float rot[3] = { DirectX::XMConvertToDegrees(DirectX::XMVectorGetX( sg->GetActiveCamera()->m_rotation_radians)),
				DirectX::XMConvertToDegrees( DirectX::XMVectorGetY(sg->GetActiveCamera()->m_rotation_radians)),
				DirectX::XMConvertToDegrees( DirectX::XMVectorGetZ(sg->GetActiveCamera()->m_rotation_radians))};

			ImGui::DragFloat3("Rotation", rot, 0.01f);
			sg->GetActiveCamera()->SetRotation(DirectX::XMVectorSet(DirectX::XMConvertToRadians(rot[0]), DirectX::XMConvertToRadians(rot[1]), DirectX::XMConvertToRadians(rot[2]), 0));

			sg->GetActiveCamera()->SignalTransformChange();
			sg->GetActiveCamera()->SignalChange();

			ImGui::End();
		}

		wr::imgui::window::LightEditor(sg);
		wr::imgui::window::ModelEditor(sg);
		wr::imgui::window::ShaderRegistry();
		wr::imgui::window::PipelineRegistry();
		wr::imgui::window::RootSignatureRegistry();
		wr::imgui::window::D3D12HardwareInfo(*render_system);
		wr::imgui::window::D3D12Settings();
	}
	
}

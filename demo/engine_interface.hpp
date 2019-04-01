#pragma once

#include "wisp.hpp"
#include "window.hpp"
#include "imgui_tools.hpp"
#include "scene_graph/scene_graph.hpp"
#include "d3d12/d3d12_renderer.hpp"
#include "imgui/ImGuizmo.h"
#include "demo_frame_graphs.hpp"

namespace engine
{

	static bool main_menu = true;
	static bool open0 = true;
	static bool open1 = true;
	static bool open_viewport = true;
	static bool open2 = true;
	static bool open_console = false;
	static bool show_imgui = true;
	static bool fullscreen = false;
	static char message_buffer[600];

	static wr::imgui::special::DebugConsole debug_console;

	void RenderEngine(ImTextureID output, wr::D3D12RenderSystem* render_system, wr::SceneGraph* sg)
	{
		debug_console.Draw("Console", &open_console);

		if (!show_imgui)
		{
			sg->GetActiveCamera()->SetAspectRatio(render_system->m_viewport.m_viewport.Width / render_system->m_viewport.m_viewport.Height);
			return;
		}

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
				ImGui::MenuItem("Viewport", nullptr, &open_viewport);
				ImGui::MenuItem("Light Editor", nullptr, &wr::imgui::window::open_light_editor);
				ImGui::MenuItem("Inspector", nullptr, &wr::imgui::window::open_inspect_editor);
				ImGui::MenuItem("Hardware Info", nullptr, &wr::imgui::window::open_hardware_info);
				ImGui::MenuItem("DirectX 12 Settings", nullptr, &wr::imgui::window::open_d3d12_settings);
				ImGui::Separator();
				wr::imgui::menu::Registries();
				ImGui::Separator();
				ImGui::MenuItem("Theme", nullptr, &open0);
				ImGui::MenuItem("ImGui Details", nullptr, &open1);
				ImGui::MenuItem("Camera Settings", nullptr, &open2);
				ImGui::EndMenu();
			}

			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 0.4));
			ImGui::Text("Current Frame Graph: %s", fg_manager::GetFrameGraphName(fg_manager::current).c_str());
			ImGui::PopStyleColor();

			ImGui::EndMainMenuBar();
		}
		
		ImGui::DockSpaceOverViewport(main_menu, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruDockspace);

		auto& io = ImGui::GetIO();

		ImVec2 viewport_pos;
		ImVec2 viewport_size;

		if (open_viewport)
		{
			ImGui::Begin("Viewport");
			
			viewport_size = ImGui::GetContentRegionAvail();
			viewport_pos = ImGui::GetCursorScreenPos();

			sg->GetActiveCamera()->SetAspectRatio(viewport_size.x / viewport_size.y);

			ImGui::Image(output, viewport_size);
			ImGui::End();
		}

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

			float rot[3] = { DirectX::XMConvertToDegrees(DirectX::XMVectorGetX(sg->GetActiveCamera()->m_rotation_radians)),
				DirectX::XMConvertToDegrees(DirectX::XMVectorGetY(sg->GetActiveCamera()->m_rotation_radians)),
				DirectX::XMConvertToDegrees(DirectX::XMVectorGetZ(sg->GetActiveCamera()->m_rotation_radians)) };

			ImGui::DragFloat3("Rotation", rot, 0.01f);

			if (!ImGui::IsMouseDown(1))
			{
				sg->GetActiveCamera()->SetPosition(pos);
				sg->GetActiveCamera()->SetRotation(DirectX::XMVectorSet(DirectX::XMConvertToRadians(rot[0]), DirectX::XMConvertToRadians(rot[1]), DirectX::XMConvertToRadians(rot[2]), 0));

				sg->GetActiveCamera()->SignalTransformChange();
				sg->GetActiveCamera()->SignalChange();
			}

			ImGui::MenuItem("Enable DOF", nullptr, &sg->GetActiveCamera()->m_enable_dof);
			ImGui::DragFloat("F number", &sg->GetActiveCamera()->m_f_number, 1.f, 1.f, 128.f);
			ImGui::DragFloat("Film size", &sg->GetActiveCamera()->m_film_size, 1.f, 25.f, 100.f);
			ImGui::DragFloat("Bokeh Shape amount", &sg->GetActiveCamera()->m_shape_amt, 0.005f, 0.f, 2.f);
			ImGui::DragInt("Aperture blades", &sg->GetActiveCamera()->m_aperture_blades, 1, 3, 7);
			ImGui::DragFloat("Focal Length", &sg->GetActiveCamera()->m_focal_length, 1.f, 1.f, 300.f);
			ImGui::DragFloat("Focal plane distance", &sg->GetActiveCamera()->m_focus_dist, 1.f, 0.f, 1000.f);

			sg->GetActiveCamera()->SetFovFromFocalLength(sg->GetActiveCamera()->m_aspect_ratio, sg->GetActiveCamera()->m_film_size);

			ImGui::End();
		}

		wr::imgui::window::LightEditor(sg, viewport_pos, viewport_size);
		wr::imgui::window::ModelEditor(sg, viewport_pos, viewport_size);
		wr::imgui::window::ShaderRegistry();
		wr::imgui::window::PipelineRegistry();
		wr::imgui::window::RootSignatureRegistry();
		wr::imgui::window::D3D12HardwareInfo(*render_system);
		wr::imgui::window::D3D12Settings();
		wr::imgui::window::Inspect(sg);
	}
	
}

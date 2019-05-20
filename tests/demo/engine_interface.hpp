#pragma once

#include "wisp.hpp"
#include "window.hpp"
#include "imgui_tools.hpp"
#include "scene_graph/scene_graph.hpp"
#include "d3d12/d3d12_renderer.hpp"
#include "imgui/ImGuizmo.h"
#include "demo_frame_graphs.hpp"
#include "render_tasks/d3d12_rtao_task.hpp"

namespace engine
{

	static bool main_menu = true;
	static bool keep_aspect_ratio = false;
	static float custom_aspect_ratio = 1280.f/720.f;
	static bool open0 = true;
	static bool open_viewport = true;
	static bool open1 = true;
	static bool rtao_settings_open = true;
	static bool open_console = false;
	static bool show_imgui = true;
	static bool fullscreen = false;
	static wr::imgui::window::Stats stats_window { false };
	static char message_buffer[600];

	static wr::imgui::special::DebugConsole debug_console;

	static wr::RTAOSettings rtao_user_settings;

	void RenderEngine(ImTextureID output, wr::D3D12RenderSystem* render_system, wr::SceneGraph* sg)
	{
		ImVec2 viewport_pos(0, 0);
		ImVec2 viewport_size(0, 0);

		debug_console.AddCommand("stats",
			[](wr::imgui::special::DebugConsole& console, std::string const&)
			{
				stats_window.m_open = !stats_window.m_open;
				console.AddLog("Toggled the stats window.");
			},
		"Toggle the statistics window");

		debug_console.Draw("Console", &open_console);

		if (!show_imgui)
		{
			stats_window.Draw(*render_system, viewport_pos);
			sg->GetActiveCamera()->SetAspectRatio(render_system->m_viewport.m_viewport.Width / render_system->m_viewport.m_viewport.Height);
		}
		else
		{
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
					ImGui::MenuItem("Scene Graph Editor", nullptr, &wr::imgui::window::open_scene_graph_editor);
					ImGui::MenuItem("Inspector", nullptr, &wr::imgui::window::open_inspector);
					ImGui::MenuItem("Hardware Info", nullptr, &wr::imgui::window::open_hardware_info);
					ImGui::MenuItem("DirectX 12 Settings", nullptr, &wr::imgui::window::open_d3d12_settings);
					ImGui::Separator();
					wr::imgui::menu::Registries();
					ImGui::Separator();
					ImGui::MenuItem("Theme", nullptr, &open0);
					ImGui::MenuItem("Statistics", nullptr, &stats_window.m_open);
					ImGui::MenuItem("Camera Settings", nullptr, &open1);
					ImGui::MenuItem("RTAO Settings", nullptr, &rtao_settings_open);
					ImGui::EndMenu();
				}

				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 0.4f));
				ImGui::Text("Current Frame Graph: %s", fg_manager::GetFrameGraphName(fg_manager::current).c_str());
				ImGui::PopStyleColor();

				ImGui::EndMainMenuBar();
			}

			ImGui::DockSpaceOverViewport(main_menu, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

			auto& io = ImGui::GetIO();

			if (open_viewport)
			{
				ImGui::Begin("Viewport");

				ImGui::Checkbox("Stats", &stats_window.m_open);
				ImGui::SameLine();
				ImGui::SetNextItemWidth(80);
				ImGui::Checkbox("Force Aspect Ratio", &keep_aspect_ratio);
				ImGui::SameLine();
				ImGui::InputFloat("##", &custom_aspect_ratio);

				viewport_pos = ImGui::GetCursorScreenPos();
				viewport_size = ImGui::GetContentRegionAvail();

				if (keep_aspect_ratio)
				{
					sg->GetActiveCamera()->SetAspectRatio(custom_aspect_ratio);
				}
				else
				{
					sg->GetActiveCamera()->SetAspectRatio(viewport_size.x / viewport_size.y);
				}

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
				if (ImGui::Button("Corporate Grey")) ImGui::StyleCorporateGrey();
				ImGui::End();
			}

			if (open1)
			{
				ImGui::Begin("Camera Settings", &open1);

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
				ImGui::DragFloat("Bokeh Shape amount", &sg->GetActiveCamera()->m_shape_amt, 0.005f, 0.f, 1.f);
				ImGui::DragInt("Aperture blades", &sg->GetActiveCamera()->m_aperture_blades, 1, 3, 7);
				ImGui::DragFloat("Focal Length", &sg->GetActiveCamera()->m_focal_length, 1.f, 1.f, 300.f);
				ImGui::DragFloat("Focal plane distance", &sg->GetActiveCamera()->m_focus_dist, 1.f, 0.f, 10000.f);

				sg->GetActiveCamera()->SetFovFromFocalLength(sg->GetActiveCamera()->m_aspect_ratio, sg->GetActiveCamera()->m_film_size);

				ImGui::End();
			}

			stats_window.Draw(*render_system, viewport_pos);
			wr::imgui::window::SceneGraphEditor(sg);
			wr::imgui::window::Inspector(sg, viewport_pos, viewport_size);
			wr::imgui::window::ShaderRegistry();
			wr::imgui::window::PipelineRegistry();
			wr::imgui::window::RootSignatureRegistry();
			wr::imgui::window::D3D12HardwareInfo(*render_system);
			wr::imgui::window::D3D12Settings();
		}

		if (rtao_settings_open)
		{
			ImGui::Begin("RTAO Settings", &rtao_settings_open);

			ImGui::DragFloat("Bias", &rtao_user_settings.m_runtime.bias, 0.01f, 0.0f, 100.f);
			ImGui::DragFloat("Radius", &rtao_user_settings.m_runtime.radius, 0.1f, 0.0f, 1000.f);
			ImGui::DragFloat("Power", &rtao_user_settings.m_runtime.power, 0.1f, 0.0f, 10.f);
			ImGui::DragInt("SPP", &rtao_user_settings.m_runtime.sample_count, 1, 0, 1073741824);

			ImGui::End();
			
			fg_manager::Get()->UpdateSettings<wr::RTAOData>(rtao_user_settings);
		}
	}
	
}

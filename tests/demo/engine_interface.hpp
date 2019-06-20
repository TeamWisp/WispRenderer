/*!
 * Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <algorithm>

#include "wisp.hpp"
#include "window.hpp"
#include "imgui_tools.hpp"
#include "scene_graph/scene_graph.hpp"
#include "d3d12/d3d12_renderer.hpp"
#include "imgui/ImGuizmo.h"
#include "demo_frame_graphs.hpp"
#include "../common/scene.hpp"
#include "scene_emibl.hpp"
#include "scene_viknell.hpp"
#include "scene_sponza.hpp"
#include "scene_alien.hpp"
#include "imgui_graphics_settings.hpp"

namespace engine
{

	static bool main_menu = true;
	static bool keep_aspect_ratio = false;
	static float custom_aspect_ratio = 1280.f/720.f;
	static bool open0 = true;
	static bool open_viewport = true;
	static bool open1 = true;
	static bool open_console = false;
	static bool open_scene = true;
	static bool open_recorder = true;
	static char recorder_name[256] = "unamed";
	static int selected_scene = 0;
	static bool show_imgui = true;
	static bool fullscreen = false;
	static wr::imgui::window::Stats stats_window { false };
	static char message_buffer[600];

	static wr::imgui::special::DebugConsole debug_console;

	struct Recorder
	{
		bool m_recording = false;
		int m_target_framerate = 30;
		int m_record_frame_inverval = 1;
		int m_frames_since_last_capture = 0;
		int m_frames_recorded = 0;
		std::string m_output_dir;
		std::string m_name;

		void Start(std::string name)
		{
			m_frames_recorded = 0;
			m_frames_since_last_capture = 0;

			m_output_dir = "D:\\WispRecorder\\" + name;
			m_name = name;

			std::filesystem::create_directory(m_output_dir);

			show_imgui = false;

			m_recording = true;
		}

		void Stop()
		{
			m_recording = false;
		}

		std::string GetNextFilename(std::string ext)
		{
			m_frames_recorded++;
			return m_output_dir + "\\" + m_name + "_"+ std::to_string(m_target_framerate) + "fps_frame" + std::to_string(m_frames_recorded) + ext;
		}

		bool ShouldCaptureAndIncrement(float& out_delta)
		{
			if (!m_recording) return false;

			bool retval = false;

			if (m_frames_since_last_capture == m_record_frame_inverval)
			{
				retval = true;
				m_frames_since_last_capture = 0;
				out_delta = 1.f / (float)m_target_framerate;;
			}
			else
			{
				m_frames_since_last_capture++;
				out_delta = 0;
			}

			return retval;
		}

		bool IsRecording()
		{
			return m_recording;
		}
	};

	static Recorder recorder;

	void RenderEngine(ImTextureID output, wr::D3D12RenderSystem* render_system, Scene* scene, Scene** new_scene)
	{
		auto sg = scene->GetSceneGraph();

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
					wr::imgui::menu::GraphicsSettingsMenu(fg_manager::Get());
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

				if (!keep_aspect_ratio && viewport_size.x > 10 && viewport_size.y > 10)
				{
					sg->GetActiveCamera()->SetAspectRatio(viewport_size.x / viewport_size.y);
				}
				else
				{
					sg->GetActiveCamera()->SetAspectRatio(custom_aspect_ratio);
				}

				ImGui::Image(output, viewport_size);
				ImGui::End();
			}

			if (open_scene)
			{
				ImGui::Begin("Demo Scene", &open_scene);
				if (ImGui::Button("Save Lights"))
				{
					scene->SaveLightsToJSON();
				}

				ImGui::Separator();

				const char* items[] = { "Viknell", "Emibl", "Sponza", "Alien" };

				ImGui::Combo("##", &selected_scene, items, IM_ARRAYSIZE(items));
				ImGui::SameLine();
				if (ImGui::Button("Load"))
				{
					switch (selected_scene)
					{
						case 0: (*new_scene) = new ViknellScene(); break;
						case 1: (*new_scene) = new EmiblScene(); break;
						case 2: (*new_scene) = new SponzaScene(); break;
						case 3: (*new_scene) = new AlienScene(); break;
						default: LOGW("Tried to load a scene that is not supported"); break;
					}
				}
				ImGui::End();
			}

			if (open_recorder)
			{
				recorder.Stop(); // don't record while in imgui.

				ImGui::Begin("Recorder", &open_recorder);
				if (ImGui::Button("Record"))
				{
					recorder.Start(recorder_name);
				}

				ImGui::InputText("Recording Name", recorder_name, IM_ARRAYSIZE(recorder_name));
				ImGui::InputInt("Target Framerate", &recorder.m_target_framerate);
				ImGui::InputInt("Frame Interval", &recorder.m_record_frame_inverval);

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

				ImGui::Separator();

				float frustum_near = sg->GetActiveCamera()->m_frustum_near;
				float frustum_far = sg->GetActiveCamera()->m_frustum_far;
				ImGui::DragFloatRange2("Frustum Near/Far", &frustum_near, &frustum_far, 1, 0.0001f, std::numeric_limits<float>::max());
				sg->GetActiveCamera()->SetFrustumNear(std::max(frustum_near, 0.0000001f));
				sg->GetActiveCamera()->SetFrustumFar(std::max(frustum_far, 0.0000001f));

				ImGui::Separator();
				
				ImGui::MenuItem("Enable DOF", nullptr, &sg->GetActiveCamera()->m_enable_dof);
				ImGui::MenuItem("Enable Orthographic view", nullptr, &sg->GetActiveCamera()->m_enable_orthographic);
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
			wr::imgui::window::SceneGraphEditor(sg.get());
			wr::imgui::window::Inspector(sg.get(), viewport_pos, viewport_size);
			wr::imgui::window::ShaderRegistry();
			wr::imgui::window::PipelineRegistry();
			wr::imgui::window::RootSignatureRegistry();
			wr::imgui::window::D3D12HardwareInfo(*render_system);
			wr::imgui::window::D3D12Settings();
			wr::imgui::window::GraphicsSettings(fg_manager::Get());
		}
	}
}

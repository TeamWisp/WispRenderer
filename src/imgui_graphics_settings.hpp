#pragma once

#include "render_tasks/d3d12_rtao_task.hpp"
#include "render_tasks/d3d12_hbao.hpp"
#include "render_tasks/d3d12_ansel.hpp"
#include "render_tasks/d3d12_build_acceleration_structures.hpp"

namespace wr::imgui::window
{

	static bool rtao_settings_open = true;


	static wr::RTAOSettings rtao_user_settings;
	static wr::HBAOSettings hbao_user_settings;
	static wr::AnselSettings ansel_user_settings;
	static wr::ASBuildSettings as_build_user_settings;

	void GraphicsSettings(wr::FrameGraph* frame_graph)
	{
		if (rtao_settings_open)
		{
			ImGui::Begin("Graphics Settings", &rtao_settings_open);

			ImGui::LabelText("", "RTAO");
			ImGui::Separator();
			ImGui::DragFloat("Bias", &rtao_user_settings.m_runtime.bias, 0.01f, 0.0f, 100.f);
			ImGui::DragFloat("Radius", &rtao_user_settings.m_runtime.radius, 0.1f, 0.0f, 1000.f);
			ImGui::DragFloat("Power", &rtao_user_settings.m_runtime.power, 0.1f, 0.0f, 10.f);
			ImGui::DragInt("SPP", &rtao_user_settings.m_runtime.sample_count, 1, 0, 1073741824);
			
			if (frame_graph->HasTask<wr::RTAOData>())
			{
				frame_graph->UpdateSettings<wr::RTAOData>(rtao_user_settings);
			}

			
			ImGui::Dummy(ImVec2(0.0f, 10.0f));
			ImGui::LabelText("", "HBAO+");
			ImGui::Separator();

			ImGui::DragFloat("Meters to units", &hbao_user_settings.m_runtime.m_meters_to_view_space_units, 0.1f, 0.1f, 100.f);
			ImGui::DragFloat("Radius", &hbao_user_settings.m_runtime.m_radius, 0.1f, 0.0f, 100.f);
			ImGui::DragFloat("Bias", &hbao_user_settings.m_runtime.m_bias, 0.1f, 0.0f, 0.5f);
			ImGui::DragFloat("Power", &hbao_user_settings.m_runtime.m_power_exp, 0.1f, 1.f, 4.f);
			ImGui::Checkbox("Blur", &hbao_user_settings.m_runtime.m_enable_blur);
			ImGui::DragFloat("Blur Sharpness", &hbao_user_settings.m_runtime.m_blur_sharpness, 0.5f, 0.0f, 16.0f);
			
			if (frame_graph->HasTask<wr::HBAOData>())
			{
				frame_graph->UpdateSettings<wr::HBAOData>(hbao_user_settings);
			}


			ImGui::Dummy(ImVec2(0.0f, 10.0f));
			ImGui::LabelText("", "Ansel");
			ImGui::Separator();

			ImGui::Checkbox("Translation", &ansel_user_settings.m_runtime.m_allow_translation); ImGui::SameLine();
			ImGui::Checkbox("Rotation", &ansel_user_settings.m_runtime.m_allow_rotation); ImGui::SameLine();
			ImGui::Checkbox("FOV", &ansel_user_settings.m_runtime.m_allow_fov); 
			ImGui::Checkbox("Mono 360", &ansel_user_settings.m_runtime.m_allow_mono_360); ImGui::SameLine();

			ImGui::Checkbox("Stereo 360", &ansel_user_settings.m_runtime.m_allow_stero_360); ImGui::SameLine();
			ImGui::Checkbox("Raw HDR", &ansel_user_settings.m_runtime.m_allow_raw);
			ImGui::Checkbox("Pause", &ansel_user_settings.m_runtime.m_allow_pause); ImGui::SameLine();
			ImGui::Checkbox("High res", &ansel_user_settings.m_runtime.m_allow_highres);


			ImGui::DragFloat("Camera Speed", &ansel_user_settings.m_runtime.m_translation_speed_in_world_units_per_sec, 0.1f, 0.1f, 100.f);
			ImGui::DragFloat("Rotation Speed", &ansel_user_settings.m_runtime.m_rotation_speed_in_deg_per_second, 5.f, 5.f, 920.f);
			ImGui::DragFloat("Max FOV", &ansel_user_settings.m_runtime.m_maximum_fov_in_deg, 1.f, 140.f, 179.f);
		
			if (frame_graph->HasTask<wr::AnselData>())
			{
				frame_graph->UpdateSettings<wr::AnselData>(ansel_user_settings);
			}

			ImGui::Dummy(ImVec2(0.0f, 10.0f));
			ImGui::LabelText("", "Acceleration Structure");
			ImGui::Separator();

			ImGui::Checkbox("Enable rebuilding", &as_build_user_settings.m_runtime.m_rebuild_as);
			if (frame_graph->HasTask<wr::ASBuildData>())
			{
				frame_graph->UpdateSettings<wr::ASBuildData>(as_build_user_settings);
			}

			ImGui::End();

		}
	}

}// namepace engine
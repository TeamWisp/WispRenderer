#pragma once

#include "render_tasks/d3d12_rtao_task.hpp"
#include "render_tasks/d3d12_hbao.hpp"
#include "render_tasks/d3d12_ansel.hpp"
#include "render_tasks/d3d12_build_acceleration_structures.hpp"
#include "render_tasks/d3d12_rt_shadow_task.hpp"
#include "render_tasks/d3d12_shadow_denoiser_task.hpp"

namespace wr::imgui::window
{
	static bool rtao_settings_open = true;
	static bool hbao_settings_open = true;
	static bool ansel_settings_open = true;
	static bool asbuild_settings_open = true;
	static bool shadow_settings_open = true;
	static bool shadow_denoiser_settings_open = true;

	void GraphicsSettings(FrameGraph* frame_graph)
	{
		if (frame_graph->HasTask<wr::RTAOData>() && rtao_settings_open)
		{
			auto rtao_user_settings = frame_graph->GetSettings<RTAOData, RTAOSettings>();
			ImGui::Begin("RTAO Settings", &rtao_settings_open);

			ImGui::DragFloat("Bias", &rtao_user_settings.m_runtime.bias, 0.01f, 0.0f, 100.f);
			ImGui::DragFloat("Radius", &rtao_user_settings.m_runtime.radius, 0.1f, 0.0f, 1000.f);
			ImGui::DragFloat("Power", &rtao_user_settings.m_runtime.power, 0.1f, 0.0f, 10.f);
			ImGui::DragInt("SPP", &rtao_user_settings.m_runtime.sample_count, 1, 0, 1073741824);

			ImGui::End();

			frame_graph->UpdateSettings<RTAOData>(rtao_user_settings);
		}


		if (frame_graph->HasTask<HBAOData>() && hbao_settings_open)
		{
			auto hbao_user_settings = frame_graph->GetSettings<HBAOData, HBAOSettings>();
			ImGui::Begin("HBAO+ Settings", &hbao_settings_open);

			ImGui::DragFloat("Meters to units", &hbao_user_settings.m_runtime.m_meters_to_view_space_units, 0.1f, 0.1f, 100.f);
			ImGui::DragFloat("Radius", &hbao_user_settings.m_runtime.m_radius, 0.1f, 0.0f, 100.f);
			ImGui::DragFloat("Bias", &hbao_user_settings.m_runtime.m_bias, 0.1f, 0.0f, 0.5f);
			ImGui::DragFloat("Power", &hbao_user_settings.m_runtime.m_power_exp, 0.1f, 1.f, 4.f);
			ImGui::Checkbox("Blur", &hbao_user_settings.m_runtime.m_enable_blur);
			ImGui::DragFloat("Blur Sharpness", &hbao_user_settings.m_runtime.m_blur_sharpness, 0.5f, 0.0f, 16.0f);

			ImGui::End();

			frame_graph->UpdateSettings<HBAOData>(hbao_user_settings);
		}


		if (frame_graph->HasTask<AnselData>() && ansel_settings_open)
		{
			auto ansel_user_settings = frame_graph->GetSettings<AnselData, AnselSettings>();

			ImGui::Begin("NVIDIA Ansel Settings", &ansel_settings_open);

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

			ImGui::End();

			frame_graph->UpdateSettings<AnselData>(ansel_user_settings);
		}


		if (frame_graph->HasTask<ASBuildData>() && asbuild_settings_open)
		{
			auto as_build_user_settings = frame_graph->GetSettings<ASBuildData, ASBuildSettings>();

			ImGui::Begin("Acceleration Structure Settings", &asbuild_settings_open);

			ImGui::Checkbox("Disable rebuilding", &as_build_user_settings.m_runtime.m_rebuild_as);

			ImGui::End();
			frame_graph->UpdateSettings<ASBuildData>(as_build_user_settings);
		}

		if (frame_graph->HasTask<RTShadowData>() && shadow_settings_open)
		{
			auto shadow_user_settings = frame_graph->GetSettings<RTShadowData, RTShadowSettings>();

			ImGui::Begin("Shadow Settings", &rtao_settings_open);

			ImGui::DragFloat("Epsilon", &shadow_user_settings.m_runtime.m_epsilon, 0.01f, 0.0f, 15.f);
			ImGui::DragInt("Sample Count", &shadow_user_settings.m_runtime.m_sample_count, 1, 1, 64);
			
			frame_graph->UpdateSettings<RTShadowData>(shadow_user_settings);
			
			if (frame_graph->HasTask<ShadowDenoiserData>())
			{
				auto shadow_denoiser_user_settings = frame_graph->GetSettings<ShadowDenoiserData, ShadowDenoiserSettings>();

				ImGui::Dummy(ImVec2(0.0f, 10.0f));
				ImGui::LabelText("", "Denoising");
				ImGui::Separator();

				ImGui::DragFloat("Alpha", &shadow_denoiser_user_settings.m_runtime.m_alpha, 0.01f, 0.001f, 1.f);
				ImGui::DragFloat("Moments Alpha", &shadow_denoiser_user_settings.m_runtime.m_moments_alpha, 0.01f, 0.001f, 1.f);
				ImGui::DragFloat("L Phi", &shadow_denoiser_user_settings.m_runtime.m_l_phi, 0.1f, 0.1f, 16.f);
				ImGui::DragFloat("N Phi", &shadow_denoiser_user_settings.m_runtime.m_n_phi, 1.f, 1.f, 360.f);
				ImGui::DragFloat("Z Phi", &shadow_denoiser_user_settings.m_runtime.m_z_phi, 0.1f, 0.1f, 16.f);

				frame_graph->UpdateSettings<ShadowDenoiserData>(shadow_denoiser_user_settings);
			}
			ImGui::End();


		}	
	}

}// namepace imgui::window

namespace wr::imgui::menu
{
	void GraphicsSettingsMenu(wr::FrameGraph* frame_graph)
	{
		if (ImGui::BeginMenu("Graphics Settings"))
		{
			if (frame_graph->HasTask<wr::RTAOData>())
			{
				ImGui::MenuItem("RTAO Settings", nullptr, &window::rtao_settings_open);
			}
			if (frame_graph->HasTask<wr::HBAOData>())
			{
				ImGui::MenuItem("HBAO Settings", nullptr, &window::hbao_settings_open);
			}
			if (frame_graph->HasTask<wr::AnselData>())
			{
				ImGui::MenuItem("Ansel Settings", nullptr, &window::ansel_settings_open);
			}
			if (frame_graph->HasTask<wr::ASBuildData>())
			{
				ImGui::MenuItem("AS Build Settings", nullptr, &window::asbuild_settings_open);
			}
			if (frame_graph->HasTask<wr::RTShadowData>())
			{
				ImGui::MenuItem("Shadow Settings", nullptr, &window::shadow_settings_open);
			}
			ImGui::EndMenu();
		}
	}
}// namespace imgui::menu

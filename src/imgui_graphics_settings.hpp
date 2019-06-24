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

#include "render_tasks/d3d12_imgui_render_task.hpp"
#include "render_tasks/d3d12_brdf_lut_precalculation.hpp"
#include "render_tasks/d3d12_deferred_main.hpp"
#include "render_tasks/d3d12_deferred_composition.hpp"
#include "render_tasks/d3d12_deferred_render_target_copy.hpp"
#include "render_tasks/d3d12_raytracing_task.hpp"
#include "render_tasks/d3d12_rt_reflection_task.hpp"
#include "render_tasks/d3d12_rt_shadow_task.hpp"
#include "render_tasks/d3d12_shadow_denoiser_task.hpp"
#include "render_tasks/d3d12_equirect_to_cubemap.hpp"
#include "render_tasks/d3d12_cubemap_convolution.hpp"
#include "render_tasks/d3d12_rtao_task.hpp"
#include "render_tasks/d3d12_post_processing.hpp"
#include "render_tasks/d3d12_build_acceleration_structures.hpp"
#include "render_tasks/d3d12_path_tracer.hpp"
#include "render_tasks/d3d12_accumulation.hpp"
#include "render_tasks/d3d12_dof_bokeh.hpp"
#include "render_tasks/d3d12_dof_bokeh_postfilter.hpp"
#include "render_tasks/d3d12_dof_coc.hpp"
#include "render_tasks/d3d12_down_scale.hpp"
#include "render_tasks/d3d12_dof_composition.hpp"
#include "render_tasks/d3d12_dof_dilate_near.hpp"
#include "render_tasks/d3d12_hbao.hpp"
#include "render_tasks/d3d12_ansel.hpp"
#include "render_tasks/d3d12_bloom_extract_bright.hpp"
#include "render_tasks/d3d12_bloom_composition.hpp"
#include "render_tasks/d3d12_bloom_horizontal_blur.hpp"
#include "render_tasks/d3d12_bloom_vertical_blur.hpp"

namespace wr::imgui::window
{
	static bool rtao_settings_open = true;
	static bool hbao_settings_open = true;
	static bool ansel_settings_open = true;
	static bool asbuild_settings_open = true;
	static bool shadow_settings_open = true;
	static bool shadow_denoiser_settings_open = true;
	static bool frame_graph_editor_open = true;

	static RenderTaskHandle selected_task = -1;

	wr::FrameGraph* fg = nullptr;
	util::Delegate<void(ImTextureID)> imgui_function;

	std::pair<std::string, std::function<void()>> available_add_functions[] =
	{
		std::pair("BRDF LUT Precalculation", [&]() {wr::AddBrdfLutPrecalculationTask(*fg); }),
		std::pair("Equire To Cubemap", [&]() {wr::AddEquirectToCubemapTask(*fg); }),
		std::pair("Cubemap Convolution", [&]() {wr::AddCubemapConvolutionTask(*fg); }),
		std::pair("Deferred Main", [&]() {wr::AddDeferredMainTask(*fg, std::nullopt, std::nullopt, false); }),
		std::pair("HBAO+", [&]() {wr::AddHBAOTask(*fg); }),
		std::pair("Deferred Composition", [&]() {wr::AddDeferredCompositionTask(*fg, std::nullopt, std::nullopt); }),
		
		//Bloom tasks
		std::pair("Bloom extract bright", [&]() {wr::AddBloomExtractBrightTask<wr::DeferredCompositionTaskData, wr::DeferredMainTaskData>(*fg); }),
		std::pair("Bloom blur horizontal", [&]() {wr::AddBloomBlurHorizontalTask<wr::BloomExtractBrightData>(*fg);}),
		std::pair("Bloom blur vertical", [&]() {wr::AddBloomBlurVerticalTask<wr::BloomBlurHorizontalData>(*fg); }),
		std::pair("Bloom Composition", [&]() {wr::AddBloomCompositionTask<wr::DeferredCompositionTaskData, wr::BloomBlurVerticalData>(*fg); }),

		// Depth of field tasks
		std::pair("DoF Cone of Confusion", [&]() {wr::AddDoFCoCTask<wr::DeferredMainTaskData>(*fg); }),
		std::pair("DoF Down Scale", [&]() {wr::AddDownScaleTask<wr::BloomCompostionData, wr::DoFCoCData>(*fg); }),
		std::pair("DoF Dilate", [&]() {wr::AddDoFDilateTask<wr::DownScaleData>(*fg); }),
		std::pair("DoF Bokeh Pass", [&]() {wr::AddDoFBokehTask<wr::DownScaleData, wr::DoFDilateData>(*fg); }),
		std::pair("DoF Bokeh Post Filter", [&]() {wr::AddDoFBokehPostFilterTask<wr::DoFBokehData>(*fg); }),
		std::pair("DoF Composition", [&]() {wr::AddDoFCompositionTask<wr::BloomCompostionData, wr::DoFBokehPostFilterData, wr::DoFCoCData>(*fg); }),

		std::pair("Post Processing", [&]() {wr::AddPostProcessingTask<wr::DoFCompositionData>(*fg); }),

		// Copy the scene render pixel data to the final render target
		std::pair("Copy render target", [&]() {wr::AddRenderTargetCopyTask<wr::PostProcessingData>(*fg); }),

		std::pair("NVIDIA Ansel", [&]() {wr::AddAnselTask(*fg); }),

		// Display ImGui
		std::pair("ImGui",[&]() { fg->AddTask<wr::ImGuiTaskData>(wr::GetImGuiTask<wr::PostProcessingData>(imgui_function), L"ImGui"); })

	};

	void GraphicsSettings(FrameGraph* frame_graph, util::Delegate<void(ImTextureID)> const& imgui_func)
	{
		fg = frame_graph;
		imgui_function = imgui_func;
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

#ifndef FG_MAX_PERFORMANCE
		if (frame_graph_editor_open)
		{
			ImGui::Begin("FrameGraph Editor", &frame_graph_editor_open);

			const std::vector<std::wstring> names = frame_graph->GetNames();

			ImVec2 size = ImGui::GetContentRegionAvail();

			if (ImGui::ListBoxHeader("##", size))
			{
				if (ImGui::Button("Reload"))
				{
					frame_graph->m_should_reload = true;
				}

				ImGui::SameLine();

				if (ImGui::Button("Add HBAO+"))
				{
				//	frame_graph->functions.push_back(hbaoptr);
				}

				for (int i = 0; i < names.size(); i++)
				{
					//setup converter
					using convert_type = std::codecvt_utf8<wchar_t>;
					std::wstring_convert<convert_type, wchar_t> converter;

					//use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
					std::string converted_str = converter.to_bytes(names[i]);

					bool pressed = ImGui::Selectable(converted_str.c_str(), selected_task == i);

					// if we don't have that node selected.
					if (pressed && selected_task != i)
					{
						selected_task = i;
					}
					// if we already have that node selected.
					else if (pressed && selected_task == i)
					{
						selected_task = -1;
					}

					// Right click menu
					if (ImGui::BeginPopupContextItem())
					{
						RenderTaskHandle right_clicked_task = i;

						if (ImGui::Button("Disable"))
						{
							frame_graph->WaitForCompletion(right_clicked_task);
							//TODO: Remove render task
						}

						ImGui::EndPopup();
					}
				}
				ImGui::ListBoxFooter();
			}
			ImGui::End();
		}
#endif //FG_MAX_PERFORMANCE
	}
}// namepace imgui::window

namespace wr::imgui::menu
{
	void GraphicsSettingsMenu(wr::FrameGraph* frame_graph)
	{
		ImGui::MenuItem("Frame Graph Editor", nullptr, &window::frame_graph_editor_open);
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

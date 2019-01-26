#pragma once

#include "wisp.hpp"
#include "window.hpp"
#include "imgui_tools.hpp"
#include "scene_graph/scene_graph.hpp"
#include "d3d12/d3d12_renderer.hpp"
#include "imgui/ImGuizmo.h"

namespace game_ui
{
	void RenderGameUI(wr::D3D12RenderSystem* render_system, wr::SceneGraph* sg, float score)
	{
		float screen_width = render_system->m_viewport.m_viewport.Width;
		float screen_height = render_system->m_viewport.m_viewport.Height;

		bool open = true;
		ImGui::SetNextWindowPos(ImVec2((screen_width / 2) - (100 / 2), 20));
		ImGui::SetNextWindowSize(ImVec2(100, 22));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.5f));
		ImGui::Begin("Game UI", &open, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove
										| ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("Score: %d", (int)ceil(score));
		ImGui::End();
		ImGui::PopStyleColor();


		// Crosshair
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(screen_width, screen_height));
		ImGui::Begin("FS Overlay", &open, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground);

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		static ImVec4 col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
		static ImVec4 bg_col = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);
		draw_list->AddCircle(ImVec2(screen_width / 2, screen_height / 2), 5, ImColor(bg_col), 32);
		draw_list->AddCircleFilled(ImVec2(screen_width / 2, screen_height / 2), 4, ImColor(col), 32);

		ImGui::End();
	}
}
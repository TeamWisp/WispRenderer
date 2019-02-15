#pragma once

#include <vector>
#include <functional>

#include "imgui/imgui.hpp"
#include "scene_graph/light_node.hpp"

namespace wr
{
	class D3D12RenderSystem;
	class SceneGraph;
}

namespace wr::imgui
{

	namespace menu
	{
		void Registries();
	}

	namespace window
	{
		void ShaderRegistry();
		void PipelineRegistry();
		void RootSignatureRegistry();
		void D3D12HardwareInfo(D3D12RenderSystem& render_system);
		void D3D12Settings();
		void LightEditor(SceneGraph* scene_graph);
		void ModelEditor(SceneGraph* scene_graph);

		static bool open_hardware_info = true;
		static bool open_d3d12_settings = true;
		static bool open_shader_registry = true;
		static bool open_shader_compiler_popup = false;
		static std::string shader_compiler_error = "No shader error";
		static bool open_pipeline_registry = true;
		static bool open_root_signature_registry = true;
		static bool open_light_editor = true;
		static bool open_model_editor = true;
		static wr::LightNode* selected_light = nullptr;
	}

	namespace special
	{
		struct DebugConsole
		{
			struct Command
			{
				using func_t = std::function<void(DebugConsole&, std::string const &)>;
				std::string m_name;
				std::string m_description;
				func_t m_function;
				bool m_starts_with = false;
			};

			char                  InputBuf[256];
			ImVector<char*>       Items;
			bool                  ScrollToBottom;
			ImVector<char*>       History;
			int                   HistoryPos;    // -1: new line, 0..History.Size-1 browsing history.
			std::vector<Command> m_commands;
			ImGuiTextFilter filter;

			DebugConsole();
			~DebugConsole();

			void EmptyInput();

			void ClearLog();

			void AddLog(const char* fmt, ...);

			void Draw(const char* title, bool* p_open);

			void AddCommand(std::string name, Command::func_t func, std::string desc = "", bool starts_with = false);

		private:
			void ExecCommand(const char* command_line);

			static int TextEditCallbackStub(ImGuiInputTextCallbackData* data);

			int TextEditCallback(ImGuiInputTextCallbackData* data);
		};
	}

} /* wr::imgui */

#pragma once

#include <vector>
#include <functional>
#include <typeindex>

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
		void SceneGraphEditor(SceneGraph* scene_graph);
		void Inspector(SceneGraph* scene_graph, ImVec2 viewport_pos, ImVec2 viewport_size);

		static bool open_hardware_info = true;
		static bool open_d3d12_settings = true;
		static bool open_shader_registry = false;
		static bool open_shader_compiler_popup = false;
		static std::string shader_compiler_error = "No shader error";
		static bool open_pipeline_registry = false;
		static bool open_root_signature_registry = false;
		static bool open_effect_editor = true;
		static bool open_scene_graph_editor = true;
		static bool open_inspector = true;
		static wr::LightNode* selected_light = nullptr;
		static bool light_selected = false;

		static std::shared_ptr<Node> selected_node = nullptr;

		struct SceneGraphEditorDetails
		{
			using name_func_t = std::function<std::string(std::shared_ptr<Node>)>;
			using inspect_func_t = std::function<void(std::shared_ptr<Node>, SceneGraph*)>;
			using context_menu_func_t = std::function<bool(std::shared_ptr<Node>, SceneGraph*)>;
			static const std::unordered_map<std::type_index, name_func_t> sg_editor_type_names;
			static const std::unordered_map<std::type_index, inspect_func_t> sg_editor_type_inspect;
			static const std::unordered_map<std::type_index, context_menu_func_t> sg_editor_type_context_menu;

			template<typename T>
			static void TryUpdateName(std::shared_ptr<Node> node, std::string& out)
			{
				if (auto t_node = std::dynamic_pointer_cast<T>(node))
				{
					if (auto it = SceneGraphEditorDetails::sg_editor_type_names.find(typeid(T)); it != SceneGraphEditorDetails::sg_editor_type_names.end())
					{
						out = it->second(node);
					}
				}
			}

			template<typename T>
			static void TryUpdateInspectFunction(std::shared_ptr<Node> node, inspect_func_t& out)
			{
				if (auto t_node = std::dynamic_pointer_cast<T>(node))
				{
					if (auto it = SceneGraphEditorDetails::sg_editor_type_inspect.find(typeid(T)); it != SceneGraphEditorDetails::sg_editor_type_inspect.end())
					{
						out = it->second;
					}
				}
			}

			template<typename T>
			static void TryUpdateContextMenuFunction(std::shared_ptr<Node> node, context_menu_func_t& out)
			{
				if (auto t_node = std::dynamic_pointer_cast<T>(node))
				{
					if (auto it = SceneGraphEditorDetails::sg_editor_type_context_menu.find(typeid(T)); it != SceneGraphEditorDetails::sg_editor_type_context_menu.end())
					{
						out = it->second;
					}
				}
			}
		};
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

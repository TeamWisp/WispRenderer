#include "imgui_tools.hpp"

#include "imgui/imgui.hpp"

#include <sstream>
#include <optional>

#include "shader_registry.hpp"
#include "pipeline_registry.hpp"
#include "root_signature_registry.hpp"
#include "d3d12/d3d12_renderer.hpp"

namespace wr::imgui::internal
{
	template <typename T>
	inline std::string ConvertPointerToStringAddress(const T* obj)
	{
		auto address(reinterpret_cast<int>(obj));
		std::stringstream ss;
		ss << std::hex << address;
		return ss.str();
	}

	template <typename T>
	inline std::string DecimalToHex(const T decimal)
	{
		std::stringstream ss;
		ss << std::hex << decimal;
		return ss.str();
	}

	inline std::string ShaderTypeToStr(ShaderType type)
	{
		switch (type)
		{
		case ShaderType::VERTEX_SHADER: return "Vertex Shader";
		case ShaderType::PIXEL_SHADER: return "Pixel Shader";
		default: return "Unknown";
		}
	}

	template<typename T>
	inline void AddressText(T* obj)
	{
		if (obj)
		{
			ImGui::Text("Address: %s", internal::ConvertPointerToStringAddress(obj).c_str());
		}
		else
		{
			ImGui::Text("Address: std::nullptr");
		}
	}

	inline std::string BooltoStr(bool val)
	{
		return val ? "True" : "False";
	}
}

namespace wr::imgui::menu
{

	void Registries()
	{
		if (ImGui::BeginMenu("Registries"))
		{
			ImGui::MenuItem("Shader Registry", nullptr, &window::open_shader_registry);
			ImGui::MenuItem("Pipeline Registry", nullptr, &window::open_pipeline_registry);
			ImGui::MenuItem("RootSignatureRegistry", nullptr, &window::open_root_signature_registry);
			ImGui::EndMenu();
		}
	}

} /* wr::imgui::menu */

namespace wr::imgui::window
{

	void D3D12HardwareInfo(D3D12RenderSystem& render_system)
	{
		auto os_info = render_system.m_device->m_sys_info;
		auto dx_info = render_system.m_device->m_adapter_info;

		if (open_hardware_info)
		{
			ImGui::Begin("Hardware Info", &open_hardware_info);
			if (ImGui::CollapsingHeader("System Information", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Text("Page Size: %i", os_info.dwPageSize);
				ImGui::Text("Application Address Range: %s - %s", internal::DecimalToHex(os_info.lpMinimumApplicationAddress).c_str(), internal::DecimalToHex(os_info.lpMaximumApplicationAddress).c_str());
				ImGui::Text("Active Processor Mask: %i", os_info.dwActiveProcessorMask);
				ImGui::Text("Processor Count: %i", os_info.dwNumberOfProcessors);

				switch (os_info.wProcessorArchitecture)
				{
				case 9: ImGui::Text("Processor Architecture: %s", "PROCESSOR_ARCHITECTURE_AMD64"); break;
				case 5: ImGui::Text("Processor Architecture: %s", "PROCESSOR_ARCHITECTURE_ARM"); break;
				case 6: ImGui::Text("Processor Architecture: %s", "PROCESSOR_ARCHITECTURE_IA64"); break;
				case 0: ImGui::Text("Processor Architecture: %s", "PROCESSOR_ARCHITECTURE_INTEL"); break;
				default: ImGui::Text("Processor Architecture: %s", "Unknown"); break;
				}
			}

			if (ImGui::CollapsingHeader("Graphics Adapter Information", ImGuiTreeNodeFlags_DefaultOpen))
			{
				std::wstring wdesc(dx_info.Description);
				std::string desc(wdesc.begin(), wdesc.end());

				ImGui::Text("Description: %s", desc.c_str());
				ImGui::Text("Vendor ID: %i", dx_info.VendorId);
				ImGui::Text("Device ID: %i", dx_info.DeviceId);
				ImGui::Text("Subsystem ID: %i", dx_info.SubSysId);
				ImGui::Text("Dedicated Video Memory: %s", internal::DecimalToHex(dx_info.DedicatedVideoMemory).c_str());
				ImGui::Text("Dedicated System Memory: %s", internal::DecimalToHex(dx_info.DedicatedSystemMemory).c_str());
				ImGui::Text("Shared System Memory: %s", internal::DecimalToHex(dx_info.SharedSystemMemory).c_str());
			}

			ImGui::End();
		}
	}

	void ShaderRegistry()
	{
		if (open_shader_registry)
		{
			auto& registry = ShaderRegistry::Get();

			ImGui::Begin("Shader Registry", &open_shader_registry);

			ImGui::Text("Num descriptors: %d", registry.m_descriptions.size());
			ImGui::Text("Num objects: %d", registry.m_objects.size());
			ImGui::Separator();

			for (auto desc : registry.m_descriptions)
			{
				Shader* obj = nullptr;
				auto obj_it = registry.m_objects.find(desc.first);
				if (obj_it != registry.m_objects.end())
				{
					obj = obj_it->second;
				}

				std::string tree_name = internal::ShaderTypeToStr(desc.second.type) + "[" + std::to_string(desc.first) + "]: " + desc.second.path;
				if (ImGui::TreeNode(tree_name.c_str()))
				{
					if (ImGui::TreeNode("Description"))
					{
						ImGui::Text("ID: %d", desc.first);
						ImGui::Text("Path: %s", desc.second.path.c_str());
						ImGui::Text("Entry: %s", desc.second.entry.c_str());
						ImGui::Text("Type: %s", internal::ShaderTypeToStr(desc.second.type).c_str());

						ImGui::TreePop();
					}

					internal::AddressText(obj);

					if (ImGui::Button("Reload"))
					{
						// ..
					}

					ImGui::TreePop();
				}
			}

			ImGui::End();
		}
	}

	void PipelineRegistry()
	{
		if (open_pipeline_registry)
		{
			auto& registry = PipelineRegistry::Get();

			ImGui::Begin("Pipeline Registry", &open_pipeline_registry);

			ImGui::Text("Num descriptors: %d", registry.m_descriptions.size());
			ImGui::Text("Num objects: %d", registry.m_objects.size());
			ImGui::Separator();

			for (auto desc : registry.m_descriptions)
			{
				Pipeline* obj = nullptr;
				auto obj_it = registry.m_objects.find(desc.first);
				if (obj_it != registry.m_objects.end())
				{
					obj = obj_it->second;
				}

				std::string tree_name = "Pipeline[" + std::to_string(desc.first) + "]";
				if (ImGui::TreeNode(tree_name.c_str()))
				{
					if (ImGui::TreeNode("Description"))
					{
						ImGui::Text("ID: %d", desc.first);

						auto text_handle = [](std::string handle_name, std::optional<int> handle)
						{
							if (handle.has_value())
							{
								std::string text = handle_name + " Handle: %d";
								ImGui::Text(text.c_str(), handle);
							}
						};

						text_handle("Vertex Shader", desc.second.m_vertex_shader_handle);
						text_handle("Pixel Shader", desc.second.m_pixel_shader_handle);
						text_handle("Compute Shader", desc.second.m_compute_shader_handle);
						text_handle("RootSignature", desc.second.m_root_signature_handle);
						
						ImGui::Text("Depth Enabled: %s", internal::BooltoStr(desc.second.m_depth_enabled).c_str());
						ImGui::Text("Counter Clockwise Winding Order: %s", internal::BooltoStr(desc.second.m_counter_clockwise).c_str());
						ImGui::Text("Num RTV Formats: %d", desc.second.m_num_rtv_formats);

						ImGui::TreePop();
					}

					internal::AddressText(obj);

					ImGui::TreePop();
				}
			}

			ImGui::End();
		}
	}

	void RootSignatureRegistry()
	{
		if (open_root_signature_registry)
		{
			auto& registry = RootSignatureRegistry::Get();

			ImGui::Begin("Root Signature Registry", &open_root_signature_registry);

			ImGui::Text("Num descriptors: %d", registry.m_descriptions.size());
			ImGui::Text("Num objects: %d", registry.m_objects.size());
			ImGui::Separator();

			for (auto desc : registry.m_descriptions)
			{
				RootSignature* obj = nullptr;
				auto obj_it = registry.m_objects.find(desc.first);
				if (obj_it != registry.m_objects.end())
				{
					obj = obj_it->second;
				}

				std::string tree_name = "Root Signature[" + std::to_string(desc.first) + "]";
				if (ImGui::TreeNode(tree_name.c_str()))
				{
					if (ImGui::TreeNode("Description"))
					{
						ImGui::Text("ID: %d", desc.first);

						auto text_handle = [](std::string handle_name, std::optional<int> handle)
						{
							if (handle.has_value())
							{
								std::string text = handle_name + " Handle: %d";
								ImGui::Text(text.c_str(), handle);
							}
						};

						ImGui::Text("Num samplers: %d", desc.second.m_samplers.size());
						ImGui::Text("Num parameters: %d", desc.second.m_parameters.size());

						ImGui::TreePop();
					}

					internal::AddressText(obj);

					ImGui::TreePop();
				}
			}

			ImGui::End();
		}
	}

} /* wr::imgui::window */

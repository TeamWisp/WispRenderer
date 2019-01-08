#include "imgui_tools.hpp"

#include "imgui/imgui_internal.hpp"

#include <sstream>
#include <optional>

#include "scene_graph/camera_node.hpp"
#include "shader_registry.hpp"
#include "pipeline_registry.hpp"
#include "root_signature_registry.hpp"
#include "d3d12/d3d12_renderer.hpp"
#include "scene_graph/scene_graph.hpp"
#include "scene_graph/light_node.hpp"
#include "scene_graph/mesh_node.hpp"
#include "model_pool.hpp"
#include "d3d12/d3d12_shader_registry.hpp"
#include "d3d12/d3d12_rt_pipeline_registry.hpp"
#include "d3d12/d3d12_pipeline_registry.hpp"
#include "imgui/ImGuizmo.h"

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

	inline std::string FeatureLevelToStr(D3D_FEATURE_LEVEL level)
	{
		switch (level)
		{
		case D3D_FEATURE_LEVEL_11_0: return "D3D_FEATURE_LEVEL_11_0";
		case D3D_FEATURE_LEVEL_11_1: return "D3D_FEATURE_LEVEL_11_1";
		case D3D_FEATURE_LEVEL_12_0: return "D3D_FEATURE_LEVEL_12_0";
		case D3D_FEATURE_LEVEL_12_1: return "D3D_FEATURE_LEVEL_12_1";
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
			if (ImGui::Button("Clear"))
			{
				render_system.clear_path = true;
			}
			ImGui::DragFloat("Metal", &render_system.temp_metal);
			ImGui::DragFloat("Rough", &render_system.temp_rough);
			ImGui::DragFloat("Radius", &render_system.light_radius);
			ImGui::DragFloat("Intensity", &render_system.temp_intensity);

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
				auto device = render_system.m_device;

				ImGui::Text("Description: %s", desc.c_str());
				ImGui::Text("Feature Level: %s", internal::FeatureLevelToStr(device->m_feature_level).c_str());
				ImGui::Text("DXR Support: %s", internal::BooltoStr(device->m_dxr_support).c_str());
				ImGui::Text("DXR Fallback Support: %s", internal::BooltoStr(device->m_dxr_fallback_support).c_str());
				ImGui::Text("Vendor ID: %i", dx_info.VendorId);
				ImGui::Text("Device ID: %i", dx_info.DeviceId);
				ImGui::Text("Subsystem ID: %i", dx_info.SubSysId);
				ImGui::Text("Dedicated Video Memory: %.3f", dx_info.DedicatedVideoMemory / 1024.f / 1024.f / 1024.f);
				ImGui::Text("Dedicated System Memory: %.3f", dx_info.DedicatedSystemMemory);
				ImGui::Text("Shared System Memory: %.3f", dx_info.SharedSystemMemory / 1024.f / 1024.f / 1024.f);
			}

			ImGui::End();
		}
	}

	void D3D12Settings()
	{
		if (open_d3d12_settings)
		{
			ImGui::Begin("DirectX 12 Settings", &open_d3d12_settings);
			ImGui::Text("Num back buffers: %d", d3d12::settings::num_back_buffers);
			ImGui::Text("Back buffer format: %s", FormatToStr(d3d12::settings::back_buffer_format).c_str());
			ImGui::Text("Shader Model: %s", d3d12::settings::default_shader_model);
			ImGui::Text("Debug Factory: %s", internal::BooltoStr(d3d12::settings::enable_debug_factory).c_str());
			ImGui::Text("Enable GPU Timeout: %s", internal::BooltoStr(d3d12::settings::enable_gpu_timeout).c_str());
			ImGui::Text("Num instances per batch: %d", d3d12::settings::num_instances_per_batch);
			ImGui::End();
		}
	}

	void LightEditor(SceneGraph* scene_graph)
	{
		if (open_light_editor)
		{
			auto& lights = scene_graph->GetLightNodes();

			ImGui::Begin("Light Editor", &open_light_editor);

			if (ImGui::Button("Add Light"))
			{
				scene_graph->CreateChild<wr::LightNode>(nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 1, 1, 1 });
			}

			ImGui::Separator();

			for (auto i = 0; i < lights.size(); i++)
			{
				std::string tree_name("Light " + std::to_string(i));
				if (ImGui::TreeNode(tree_name.c_str()))
				{
					auto& light_node = lights[i];
					auto& light = *light_node->m_light;

					const char* listbox_items[] = { "Point Light", "Directional Light", "Spot Light" };
					int type = (int)light.tid & 3;
					ImGui::Combo("Type", &type, listbox_items, 3);
					light.tid = type;
					
					if (i == 0)
						light.tid |= (uint32_t) lights.size() << 2;

					ImGui::DragFloat3("Color", &light.col.x, 0.25f);
					ImGui::DragFloat3("Position", light_node->m_position.m128_f32, 0.25f);

					if (type != (uint32_t)LightType::POINT)
					{		
						float rot[3] = { DirectX::XMConvertToDegrees(DirectX::XMVectorGetX(light_node->m_rotation_radians)),
						DirectX::XMConvertToDegrees(DirectX::XMVectorGetY(light_node->m_rotation_radians)),
						DirectX::XMConvertToDegrees(DirectX::XMVectorGetZ(light_node->m_rotation_radians)) };
						ImGui::DragFloat3("Rotation", rot, 0.01f);
						light_node->SetRotation(DirectX::XMVectorSet(DirectX::XMConvertToRadians(rot[0]), DirectX::XMConvertToRadians(rot[1]), DirectX::XMConvertToRadians(rot[2]), 0));

					}

					if (type != (uint32_t) LightType::DIRECTIONAL)
					{
						ImGui::DragFloat("Radius", &light.rad, 0.25f);
					}

					if (type == (uint32_t)LightType::SPOT)
					{
						light.ang = light.ang * 180.f / 3.1415926535f;
						ImGui::DragFloat("Angle", &light.ang);
						light.ang = light.ang / 180.f * 3.1415926535f;
					}

					light_node->SignalTransformChange();
					light_node->SignalChange();

					if (ImGui::Button("Remove"))
					{
						scene_graph->DestroyNode<LightNode>(lights[i]);
					}

					ImGui::TreePop();
				}
			}

			ImGui::End();

			auto ml = lights[0];
			DirectX::XMFLOAT4X4 rmat;
			auto mat = DirectX::XMMatrixTranslationFromVector(ml->m_position);
			DirectX::XMStoreFloat4x4(&rmat, mat);

			rmat._42 *= -1;

			auto cam = scene_graph->GetActiveCamera();
			DirectX::XMFLOAT4X4 rview;
			DirectX::XMFLOAT4X4 rproj;
			DirectX::XMStoreFloat4x4(&rproj, cam->m_projection);
			DirectX::XMStoreFloat4x4(&rview, cam->m_view);


			ImGuiIO& io = ImGui::GetIO();
			ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
			ImGuizmo::Manipulate(&rview._11, &rproj._11, ImGuizmo::OPERATION::TRANSLATE, ImGuizmo::MODE::WORLD, &rmat._11, NULL, NULL);

			ml->m_position = { rmat._41, rmat._42*-1, rmat._43 };

			ml->SignalTransformChange();
			ml->SignalChange();
		}
	}

	void ModelEditor(SceneGraph * scene_graph)
	{
		/*if (open_model_editor)
		{
			auto& models = scene_graph->GetMeshNodes();

			ImGui::Begin("Model Editor", &open_model_editor);

			static std::vector<char> text(256);
			

			ImGui::InputText("Model Path", text.data(), text.size());

			if (ImGui::Button("Add Model"))
			{

				//Model* model = scene_graph->GetModelPool()->Load<Vertex>(
				//	scene_graph->GetMaterialPool().get(),
				//	scene_graph->GetTexturePool().get(),
				//	text.data(), ModelType::FBX);

				//if (model != nullptr) {
				//	scene_graph->CreateChild<wr::MeshNode>(nullptr, model);
				//}
			}

			ImGui::Separator();

			std::vector<std::pair<std::string, int>> model_count;

			for (int i = 0; i < models.size(); ++i)
			{
				auto model = models[i];

				bool found = false;

				int j;

				for (j = 0; j < model_count.size(); ++j) 
				{
					if (model_count[j].first.compare(model->m_model->m_model_name) == 0) 
					{
						found = true;
						model_count[j].second++;
						break;
					}
				}

				std::string model_name;

				if (!found)
				{
					model_count.push_back(std::make_pair(model->m_model->m_model_name, 0));
					model_name = "Model: " + model->m_model->m_model_name + " " + std::to_string(0);
				}
				else
				{
					model_count.push_back(std::make_pair(model->m_model->m_model_name, 0));
					model_name = "Model: " + model->m_model->m_model_name + " " + std::to_string(model_count[j].second);
				}
				

				if (ImGui::TreeNode(model_name.c_str()))
				{

					float pos[3] = { DirectX::XMVectorGetX(model->m_position),
						DirectX::XMVectorGetY(model->m_position),
						DirectX::XMVectorGetZ(model->m_position) };
					ImGui::DragFloat3("Position", pos);
					model->SetPosition(DirectX::XMVectorSet(pos[0], pos[1], pos[2], 1));
					
					float rot[3] = { DirectX::XMConvertToDegrees(DirectX::XMVectorGetX(model->m_rotation_radians)),
						DirectX::XMConvertToDegrees(DirectX::XMVectorGetY(model->m_rotation_radians)),
						DirectX::XMConvertToDegrees(DirectX::XMVectorGetZ(model->m_rotation_radians)) };
					ImGui::DragFloat3("Rotation", rot, 0.01f);
					model->SetRotation(DirectX::XMVectorSet(DirectX::XMConvertToRadians(rot[0]), DirectX::XMConvertToRadians(rot[1]), DirectX::XMConvertToRadians(rot[2]), 0));

					float scl[3] = { DirectX::XMVectorGetX(model->m_scale),
						DirectX::XMVectorGetY(model->m_scale),
						DirectX::XMVectorGetZ(model->m_scale) };
					ImGui::DragFloat3("Scale", scl);
					model->SetScale(DirectX::XMVectorSet(scl[0], scl[1], scl[2], 1));

					if (ImGui::Button("Remove"))
					{
						int c = 0;
						for (int j = 0; j < models.size(); ++j) 
						{
							if (model->m_model == models[j]->m_model)
								c++;

						}
						if (c == 1)
							model->m_model->m_model_pool->Destroy(model->m_model);
						scene_graph->DestroyNode(model);
					}

					ImGui::SameLine();

					if (ImGui::Button("Duplicate"))
					{
						auto node = scene_graph->CreateChild<wr::MeshNode>(nullptr, model->m_model);
						node->SetPosition(model->m_position);
						node->SetRotation(model->m_rotation_radians);
						node->SetScale(model->m_scale);
					}

					ImGui::TreePop();
				}
				
			}

			ImGui::End();

		}*/
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

			ImGui::Text("Num descriptions: %d", registry.m_descriptions.size());
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
						for (auto i = 0; i < desc.second.m_num_rtv_formats; i++)
						{
							std::string text = "Format[" + std::to_string(i) + "]: %s";
							ImGui::Text(text.c_str(), FormatToStr(desc.second.m_rtv_formats[i]).c_str());
						}

						ImGui::TreePop();
					}

					internal::AddressText(obj);

					if (ImGui::Button("Reload"))
					{
						std::optional<std::string> error_msg = std::nullopt;
						auto n_pipeline = static_cast<D3D12Pipeline*>(obj)->m_native;

						auto recompile_shader = [&error_msg](auto& pipeline_shader)
						{
							auto new_shader_variant = d3d12::LoadShader(pipeline_shader->m_type,
								pipeline_shader->m_path,
								pipeline_shader->m_entry);

							if (std::holds_alternative<d3d12::Shader*>(new_shader_variant))
							{
								pipeline_shader = std::get<d3d12::Shader*>(new_shader_variant);
							}
							else
							{
								error_msg = std::get<std::string>(new_shader_variant);
							}
						};

						// Vertex Shader
						{
							recompile_shader(n_pipeline->m_vertex_shader);
						}
						// Pixel Shader
						if (!error_msg.has_value()) {
							recompile_shader(n_pipeline->m_pixel_shader);
						}

						if (error_msg.has_value())
						{
							open_shader_compiler_popup = true;
							shader_compiler_error = error_msg.value();
						}
						else
						{
							d3d12::RefinalizePipeline(n_pipeline);
						}
					}

					ImGui::TreePop();
				}
			}

			auto& rt_registry = RTPipelineRegistry::Get();

			ImGui::Separator();

			ImGui::Text("Num raytracing descriptions: %d", rt_registry.m_descriptions.size());
			ImGui::Text("Num raytracing objects: %d", rt_registry.m_objects.size());
			ImGui::Separator();

			for (auto desc : rt_registry.m_descriptions)
			{
				StateObject* obj = nullptr;
				auto obj_it = rt_registry.m_objects.find(desc.first);
				if (obj_it != rt_registry.m_objects.end())
				{
					obj = obj_it->second;
				}

				std::string tree_name = "Raytracing Pipeline[" + std::to_string(desc.first) + "]";
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

						text_handle("Library Shader", desc.second.library_desc.shader_handle);
						text_handle("Global RootSignature", desc.second.global_root_signature.value_or(-1));

						if (desc.second.local_root_signatures.has_value())
						{
							for (auto i = 0; i < desc.second.local_root_signatures.value().size(); i++)
							{
								ImGui::Text("Local Root Signature [%d] = %d", i, desc.second.local_root_signatures.value()[i]);
							}
						}

						ImGui::TreePop();
					}

					internal::AddressText(obj);

					if (ImGui::Button("Reload"))
					{
						std::optional<std::string> error_msg = std::nullopt;
						auto n_pipeline = static_cast<D3D12StateObject*>(obj)->m_native;

						auto recompile_shader = [&error_msg](auto& pipeline_shader)
						{
							auto new_shader_variant = d3d12::LoadShader(pipeline_shader->m_type,
								pipeline_shader->m_path,
								pipeline_shader->m_entry);

							if (std::holds_alternative<d3d12::Shader*>(new_shader_variant))
							{
								pipeline_shader = std::get<d3d12::Shader*>(new_shader_variant);
							}
							else
							{
								error_msg = std::get<std::string>(new_shader_variant);
							}
						};

						// Vertex Shader
						{
							recompile_shader(n_pipeline->m_desc.m_library);
						}

						if (error_msg.has_value())
						{
							open_shader_compiler_popup = true;
							shader_compiler_error = error_msg.value();
						}
						else
						{
							//*n_pipeline = *CreateStateObject(n_pipeline->m_device, n_pipeline->m_desc);
							d3d12::RecreateStateObject(n_pipeline);
						}
					}

					ImGui::TreePop();
				}
			}


			if (open_shader_compiler_popup)
			{
				ImGui::OpenPopup("DirectXCompiler Output");
				open_shader_compiler_popup = false;
			}

			auto viewport_size = ImGui::GetMainViewport()->Size;

			ImGui::SetNextWindowSize(viewport_size);
			if (ImGui::BeginPopupModal("DirectXCompiler Output", nullptr, ImGuiWindowFlags_AlwaysAutoResize |
				ImGuiWindowFlags_NoSavedSettings |
				ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoDocking))
			{
				ImGui::Text("%s", shader_compiler_error.c_str());

				ImGui::Separator();

				if (ImGui::IsKeyReleased(ImGuiKey_Enter) || ImGui::Button("OK")) { ImGui::CloseCurrentPopup(); }
				ImGui::EndPopup();
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

namespace wr::imgui::special
{

	DebugConsole::DebugConsole()
	{
		ClearLog();
		memset(InputBuf, 0, sizeof(InputBuf));
		HistoryPos = -1;

		AddCommand("help",
			[&](DebugConsole& console, std::string const & str)
			{
				AddLog("Commands:");
				for (auto& cmd : m_commands)
				{
					if (cmd.m_starts_with)
					{
						AddLog("- %s[param] // %s", cmd.m_name.c_str(), cmd.m_description.c_str());
					}
					else
					{
						AddLog("- %s // %s", cmd.m_name.c_str(), cmd.m_description.c_str());
					}
				}
			},
			"Shows all available commands."
		);

		AddCommand("clear",
			[&](DebugConsole& console, std::string const & str) { ClearLog(); }, "Clears the console."
		);
		AddCommand("cls",
			[&](DebugConsole& console, std::string const & str) { ClearLog(); }, "Clears the console."
		);


		AddCommand("history",
			[&](DebugConsole& console, std::string const & str)
			{
				int first = History.Size - 10;
				for (int i = first > 0 ? first : 0; i < History.Size; i++)
				{
					AddLog("%3d: %s\n", i, History[i]);
				}
			},
			"Shows command history."
		);

		AddCommand("filter",
			[&](DebugConsole& console, std::string const & str)
			{
				filter.SetInputFilter();
				AddLog("Cleared Filter");
			},
			"Clears the filter.",
			false
		);

		AddCommand("filter ",
			[&](DebugConsole& console, std::string const & str)
			{
				auto params = str;
				params.erase(0, 7);

				filter.SetInputFilter(params.c_str());

				AddLog("Set filter to: %s", params.c_str());
			},
			"Sets the parameter as a filter for to console.",
			true
		);

	}

	DebugConsole::~DebugConsole()
	{
		ClearLog();
		for (int i = 0; i < History.Size; i++)
			free(History[i]);
	}

	void DebugConsole::EmptyInput()
	{
		memset(InputBuf, 0, sizeof(InputBuf));
	}

	// Portable helpers
	static bool StrEquals(std::string const & a, std::string const & b)
	{
		std::string la(a);
		std::string lb(b);
		std::transform(la.begin(), la.end(), la.begin(), ::tolower);
		std::transform(lb.begin(), lb.end(), lb.begin(), ::tolower);

		return la == lb;
	}

	static bool StrStartsWith(std::string const & str, std::string const & other)
	{
		std::string lstr(str);
		std::transform(lstr.begin(), lstr.end(), lstr.begin(), ::tolower);

		return (lstr.find(other) == 0);
	}

	static int   Stricmp(const char* str1, const char* str2) { int d; while ((d = toupper(*str2) - toupper(*str1)) == 0 && *str1) { str1++; str2++; } return d; }
	static int   Strnicmp(const char* str1, const char* str2, int n) { int d = 0; while (n > 0 && (d = toupper(*str2) - toupper(*str1)) == 0 && *str1) { str1++; str2++; n--; } return d; }
	static char* Strdup(const char *str) { size_t len = strlen(str) + 1; void* buff = malloc(len); return (char*)memcpy(buff, (const void*)str, len); }
	static void  Strtrim(char* str) { char* str_end = str + strlen(str); while (str_end > str && str_end[-1] == ' ') str_end--; *str_end = 0; }

	void DebugConsole::ClearLog()
	{
		for (int i = 0; i < Items.Size; i++)
			free(Items[i]);
		Items.clear();
		ScrollToBottom = true;
	}

	void DebugConsole::AddLog(const char* fmt, ...)
	{
		// FIXME-OPT
		char buf[1024];
		va_list args;
		va_start(args, fmt);
		vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
		buf[IM_ARRAYSIZE(buf) - 1] = 0;
		va_end(args);
		Items.push_back(Strdup(buf));
		ScrollToBottom = true;
	}

	void DebugConsole::Draw(const char* title, bool* p_open)
	{
		if (!*p_open) return;

		ImVec4 text_col(1, 1, 1, 1);
		ImVec4 bg_col(0.f, 0.f, 0.f, 0.5f);
		ImVec4 frame_col(0, 0, 0, 0);

		ImGuiContext& g = *ImGui::GetCurrentContext();
		ImGuiViewport* viewport = g.Viewports[0];

		ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 200), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(0, 0));

		ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_col);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, frame_col);

		if (!ImGui::Begin(title, p_open,
			ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize))
		{
			ImGui::End();
			return;
		}

		ImGui::SetWindowFocus();

		// As a specific feature guaranteed by the library, after calling Begin() the last Item represent the title bar. So e.g. IsItemHovered() will return true when hovering the title bar.
		// Here we create a context menu only available from the title bar.
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Close Console"))
				*p_open = false;
			ImGui::EndPopup();
		}

		const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing(); // 1 separator, 1 input text
		ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar); // Leave room for 1 separator + 1 InputText
		if (ImGui::BeginPopupContextWindow())
		{
			if (ImGui::Selectable("Clear")) ClearLog();
			ImGui::EndPopup();
		}

		// Display every line as a separate entry so we can change their color or add custom widgets. If you only want raw text you can use ImGui::TextUnformatted(log.begin(), log.end());
		// NB- if you have thousands of entries this approach may be too inefficient and may require user-side clipping to only process visible items.
		// You can seek and display only the lines that are visible using the ImGuiListClipper helper, if your elements are evenly spaced and you have cheap random access to the elements.
		// To use the clipper we could replace the 'for (int i = 0; i < Items.Size; i++)' loop with:
		//     ImGuiListClipper clipper(Items.Size);
		//     while (clipper.Step())
		//         for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
		// However, note that you can not use this code as is if a filter is active because it breaks the 'cheap random-access' property. We would need random-access on the post-filtered list.
		// A typical application wanting coarse clipping and filtering may want to pre-compute an array of indices that passed the filtering test, recomputing this array when user changes the filter,
		// and appending newly elements as they are inserted. This is left as a task to the user until we can manage to improve this example code!
		// If your items are of variable size you may want to implement code similar to what ImGuiListClipper does. Or split your data into fixed height items to allow random-seeking into your list.
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing

		for (int i = 0; i < Items.Size; i++)
		{
			const char* item = Items[i];
			if (!filter.PassFilter(item))
				continue;

			ImVec4 col = text_col;
			if (strstr(item, "[W]")) col = ImColor(1.f, 128.f / 255.f, 0.f, 1.0f);
			if (strstr(item, "[C]")) col = ImColor(1.0f, 0.4f, 0.4f, 1.0f);
			if (strstr(item, "[E]")) col = ImColor(1.0f, 0.4f, 0.4f, 1.0f);
			else if (strncmp(item, "# ", 2) == 0) col = ImColor(1.0f, 0.78f, 0.58f, 1.0f);
			ImGui::PushStyleColor(ImGuiCol_Text, col);
			ImGui::TextUnformatted(item);
			ImGui::PopStyleColor();
		}

		if (ScrollToBottom)
			ImGui::SetScrollHereY(1.0f);
		ScrollToBottom = false;
		ImGui::PopStyleVar();
		ImGui::EndChild();
		ImGui::Separator();

		// Command-line
		ImGui::SetKeyboardFocusHere(0);
		if (ImGui::InputText("", InputBuf, IM_ARRAYSIZE(InputBuf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory, &TextEditCallbackStub, (void*)this))
		{
			char* s = InputBuf;
			Strtrim(s);
			if (s[0])
				ExecCommand(s);
			strcpy(s, "");

		}

		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
	}

	void DebugConsole::AddCommand(std::string name, Command::func_t func, std::string desc, bool starts_with)
	{
		m_commands.push_back({name, desc, func, starts_with});
	}

	void DebugConsole::ExecCommand(const char* command_line)
	{
		AddLog("# %s\n", command_line);

		// Insert into history. First find match and delete it so it can be pushed to the back. This isn't trying to be smart or optimal.
		HistoryPos = -1;
		for (int i = History.Size - 1; i >= 0; i--)
			if (Stricmp(History[i], command_line) == 0)
			{
				free(History[i]);
				History.erase(History.begin() + i);
				break;
			}
		History.push_back(Strdup(command_line));

		for (auto& cmd : m_commands)
		{
			if (!cmd.m_starts_with && StrEquals(command_line, cmd.m_name))
			{
				cmd.m_function(*this, command_line);
				return;
			}
			else if (cmd.m_starts_with && StrStartsWith(command_line, cmd.m_name))
			{
				cmd.m_function(*this, command_line);
				return;
			}
		}
		AddLog("Unknown command: '%s'\n", command_line);
	}

	int DebugConsole::TextEditCallbackStub(ImGuiInputTextCallbackData* data) // In C++11 you are better off using lambdas for this sort of forwarding callbacks
	{
		DebugConsole* console = (DebugConsole*)data->UserData;
		return console->TextEditCallback(data);
	}

	int DebugConsole::TextEditCallback(ImGuiInputTextCallbackData* data)
	{
		//AddLog("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
		switch (data->EventFlag)
		{
		case ImGuiInputTextFlags_CallbackCompletion:
		{
			// Example of TEXT COMPLETION

			// Locate beginning of current word
			const char* word_end = data->Buf + data->CursorPos;
			const char* word_start = word_end;
			while (word_start > data->Buf)
			{
				const char c = word_start[-1];
				if (c == ' ' || c == '\t' || c == ',' || c == ';')
					break;
				word_start--;
			}

			// Build a list of candidates
			ImVector<const char*> candidates;
			for (int i = 0; i < m_commands.size(); i++)
				if (Strnicmp(m_commands[i].m_name.c_str(), word_start, (int)(word_end - word_start)) == 0)
					candidates.push_back(m_commands[i].m_name.c_str());

			if (candidates.Size == 0)
			{
				// No match
				AddLog("No match for \"%.*s\"!\n", (int)(word_end - word_start), word_start);
			}
			else if (candidates.Size == 1)
			{
				// Single match. Delete the beginning of the word and replace it entirely so we've got nice casing
				data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
				data->InsertChars(data->CursorPos, candidates[0]);
				data->InsertChars(data->CursorPos, " ");
			}
			else
			{
				// Multiple matches. Complete as much as we can, so inputing "C" will complete to "CL" and display "CLEAR" and "CLASSIFY"
				int match_len = (int)(word_end - word_start);
				for (;;)
				{
					int c = 0;
					bool all_candidates_matches = true;
					for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
						if (i == 0)
							c = toupper(candidates[i][match_len]);
						else if (c == 0 || c != toupper(candidates[i][match_len]))
							all_candidates_matches = false;
					if (!all_candidates_matches)
						break;
					match_len++;
				}

				if (match_len > 0)
				{
					data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
					data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
				}

				// List matches
				AddLog("Possible matches:\n");
				for (int i = 0; i < candidates.Size; i++)
					AddLog("- %s\n", candidates[i]);
			}

			break;
		}
		case ImGuiInputTextFlags_CallbackHistory:
		{
			// Example of HISTORY
			const int prev_history_pos = HistoryPos;
			if (data->EventKey == ImGuiKey_UpArrow)
			{
				if (HistoryPos == -1)
					HistoryPos = History.Size - 1;
				else if (HistoryPos > 0)
					HistoryPos--;
			}
			else if (data->EventKey == ImGuiKey_DownArrow)
			{
				if (HistoryPos != -1)
					if (++HistoryPos >= History.Size)
						HistoryPos = -1;
			}

			// A better implementation would preserve the data on the current input line along with cursor position.
			if (prev_history_pos != HistoryPos)
			{
				const char* history_str = (HistoryPos >= 0) ? History[HistoryPos] : "";
				data->DeleteChars(0, data->BufTextLen);
				data->InsertChars(0, history_str);
			}
		}
		}
		return 0;
	}

} /* wr::imgui::special */

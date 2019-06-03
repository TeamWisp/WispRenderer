#include "spline_node.hpp"

#include <imgui_tools.hpp>
#include <scene_graph/camera_node.hpp>
#include <scene_graph/scene_graph.hpp>
#include <fstream>
#include <queue>

static bool initialized = false;

SplineNode::SplineNode(std::string name, bool looping) : Node(typeid(SplineNode)), m_name(name), m_animate(false), m_speed(1), m_time(0), m_spline(nullptr), m_quat_spline(nullptr), m_looping(looping)
{
	if (initialized) return;
	initialized = true;

	wr::imgui::window::SceneGraphEditorDetails::sg_editor_type_names[typeid(SplineNode)] =
	{
		[](std::shared_ptr<Node> node) -> std::string {
			auto spline_node = std::static_pointer_cast<SplineNode>(node);
			return spline_node->m_name;
		}
	};

	wr::imgui::window::SceneGraphEditorDetails::sg_editor_type_inspect[typeid(SplineNode)] =
	{
		[](std::shared_ptr<Node> node, wr::SceneGraph* scene_graph) 
		{
			auto spline_node = std::static_pointer_cast<SplineNode>(node);

			ImGui::Checkbox("Animate", &spline_node->m_animate);
			ImGui::DragFloat("Speed", &spline_node->m_speed);
			ImGui::DragFloat("Time	", &spline_node->m_time);

			if (ImGui::Button("Save Spline"))
			{
				auto result = spline_node->SaveDialog();

				if (result.has_value())
				{
					spline_node->SaveSplineToFile(result.value());
				}
				else
				{
					LOGW("The Open dialog didn't return a file name.");
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Load Spline"))
			{
				auto result = spline_node->LoadDialog();

				if (result.has_value())
				{
					spline_node->LoadSplineFromFile(result.value());
				}
				else
				{
					LOGW("The Open dialog didn't return a file name.");
				}
			}

			if (ImGui::Button("Add Control Point"))
			{
				ControlPoint cp{};
				cp.m_position = scene_graph->GetActiveCamera()->m_position;
				cp.m_rotation = scene_graph->GetActiveCamera()->m_rotation_radians;

				spline_node->m_control_points.push_back(cp);
			}

			ImGui::Separator();

			for (std::size_t i = 0; i < spline_node->m_control_points.size(); i++)
			{
				auto i_str = std::to_string(i);

				if (ImGui::TreeNode(("Control Point " + i_str).c_str()))
				{
					auto& cp = spline_node->m_control_points[i];
					ImGui::DragFloat3(("Pos##" + i_str).c_str(), cp.m_position.m128_f32);
					ImGui::DragFloat3(("Rot##" + i_str).c_str(), cp.m_rotation.m128_f32);

					if (ImGui::Button("Take Camera Transform"))
					{
						cp.m_position = scene_graph->GetActiveCamera()->m_position;
						cp.m_rotation = scene_graph->GetActiveCamera()->m_rotation_radians;
						
					}

					if (ImGui::Button("Teleport"))
					{
						scene_graph->GetActiveCamera()->SetPosition(cp.m_position);
						scene_graph->GetActiveCamera()->SetRotation(cp.m_rotation);
					}

					if (ImGui::Button("Remove"))
					{
						spline_node->m_control_points.erase(spline_node->m_control_points.begin() + i);
					}

					ImGui::TreePop();
				}
			}

			spline_node->UpdateNaturalSpline();
		}
	};
}

SplineNode::~SplineNode()
{
	delete m_spline;
}

void SplineNode::UpdateSplineNode(float delta, std::shared_ptr<wr::Node> node)
{
	if (!m_spline || !m_quat_spline || !m_animate) return; // Don't try to run this code if we don't have a spline.

	auto impl = [&](auto spline, auto quat_spline)
	{
		m_time += delta * m_speed;
		m_time = std::fmod(m_time, spline->getMaxT());

		auto new_pos = spline->getPosition(m_time);
		auto new_rot = quat_spline->getPosition(m_time);

		node->SetPosition({ new_pos[0], new_pos[1], new_pos[2] });
		node->SetRotationQuaternion({ new_rot[0], new_rot[1], new_rot[2], new_rot[3] });
	};

	if (m_looping)
	{
		impl(reinterpret_cast<LoopingNaturalSpline<Vector<3>>*>(m_spline), reinterpret_cast<LoopingNaturalSpline<Vector<4>>*>(m_quat_spline));
	}
	else
	{
		impl(reinterpret_cast<NaturalSpline<Vector<3>>*>(m_spline), reinterpret_cast<NaturalSpline<Vector<4>>*>(m_quat_spline));
	}
}

void SplineNode::UpdateNaturalSpline()
{
    delete m_spline;
    m_spline = nullptr;

    delete m_quat_spline;
    m_quat_spline = nullptr;

	if (m_control_points.size() < (m_looping ? 2 : 3))
	{
		return;
	}

	{
		std::vector<Vector<3>> m_spline_positions;

		for (auto const& cp : m_control_points)
		{
			m_spline_positions.push_back(Vector<3>({ cp.m_position.m128_f32[0], cp.m_position.m128_f32[1], cp.m_position.m128_f32[2] }));
		}

		if (m_looping)
		{
			m_spline = new LoopingNaturalSpline<Vector<3>>(m_spline_positions);
		}
		else
		{
			m_spline = new NaturalSpline<Vector<3>>(m_spline_positions);
		}
	}

	{
		std::vector<Vector<4>> m_spline_rotations;

		for (auto const& cp : m_control_points)
		{
			auto quat = DirectX::XMQuaternionRotationRollPitchYawFromVector(cp.m_rotation);
			m_spline_rotations.push_back(Vector<4>({ quat.m128_f32[0], quat.m128_f32[1], quat.m128_f32[2], quat.m128_f32[3] }));
		}

		if (m_looping)
		{
			m_quat_spline = new LoopingNaturalSpline<Vector<4>>(m_spline_rotations);
		}
		else
		{
			m_quat_spline = new NaturalSpline<Vector<4>>(m_spline_rotations);
		}
	}
}

std::optional<std::string> SplineNode::LoadDialog()
{
	char buffer[MAX_PATH];

	OPENFILENAME dialog_args;
	ZeroMemory(&dialog_args, sizeof(dialog_args));
	dialog_args.lStructSize = sizeof(OPENFILENAME);
	dialog_args.hwndOwner = nullptr;
	dialog_args.lpstrFile = buffer;
	dialog_args.lpstrFile[0] = '\0';
	dialog_args.nMaxFile = sizeof(buffer);
	dialog_args.lpstrFilter = "All\0*.*\0Spline\0*.SPL\0";
	dialog_args.nFilterIndex = 1;
	dialog_args.lpstrTitle = "Load Spline";
	dialog_args.lpstrFileTitle = nullptr;
	dialog_args.nMaxFileTitle = 0;
	dialog_args.lpstrInitialDir = nullptr;
	dialog_args.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	bool file_selected = GetOpenFileName(&dialog_args);

	if (file_selected)
	{
		return std::string(buffer);
	}

	return std::nullopt;
}

std::optional<std::string> SplineNode::SaveDialog()
{
	char buffer[MAX_PATH] = "spline.spl";;

	OPENFILENAME dialog_args;
	ZeroMemory(&dialog_args, sizeof(dialog_args));
	dialog_args.lStructSize = sizeof(OPENFILENAME);
	dialog_args.hwndOwner = nullptr;
	dialog_args.lpstrFile = buffer;
	//dialog_args.lpstrFile[0] = '\0';
	dialog_args.nMaxFile = sizeof(buffer);
	dialog_args.lpstrFilter = "Spline\0*.SPL\0";
	dialog_args.nFilterIndex = 1;
	dialog_args.lpstrFileTitle = nullptr;
	dialog_args.lpstrTitle = "Save Spline";
	dialog_args.nMaxFileTitle = 0;
	dialog_args.lpstrInitialDir = nullptr;
	dialog_args.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	bool file_selected = GetSaveFileName(&dialog_args);

	if (file_selected)
	{
		return std::string(buffer);
	}

	return std::nullopt;
}

void SplineNode::SaveSplineToFile(std::string const & path)
{
	std::ofstream file(path);
	
	for (auto cp : m_control_points)
	{
		file << cp.m_position.m128_f32[0] << "," << cp.m_position.m128_f32[1] << "," << cp.m_position.m128_f32[2] << ",";
		file << cp.m_rotation.m128_f32[0] << "," << cp.m_rotation.m128_f32[1] << "," << cp.m_rotation.m128_f32[2] << ",\n";
	}

	file.close();
}

std::vector<std::string> Split(const std::string& subject)
{
	std::istringstream ss{ subject };
	using StrIt = std::istream_iterator<std::string>;
	std::vector<std::string> container{ StrIt{ss}, StrIt{} };
	return container;
}

void SplineNode::LoadSplineFromFile(std::string const& path)
{
	std::ifstream file(path);

	std::string str;
	std::deque<float> numbers;
	while (std::getline(file, str, ','))
	{
		numbers.push_back(std::atof(str.c_str()));
	}

	m_control_points.clear();

	while (numbers.size() > 5)
	{
		ControlPoint cp{};
		cp.m_position = { numbers[0], numbers[1], numbers[2] };
		cp.m_rotation = { numbers[3], numbers[4], numbers[5] };
		m_control_points.push_back(cp);
		 
		numbers.erase(numbers.begin(), numbers.begin() + 6);
	}

	file.close();
}

#include "spline_node.hpp"

#include <imgui_tools.hpp>
#include <scene_graph/camera_node.hpp>
#include <scene_graph/scene_graph.hpp>
#include <fstream>
#include <queue>

static bool initialized = false;

SplineNode::SplineNode(std::string name) : Node(typeid(SplineNode)), m_name(name), m_animate(false), m_speed(1), m_time(0), m_spline(nullptr), m_quat_spline(nullptr)
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
		[&](std::shared_ptr<Node> node, wr::SceneGraph* scene_graph) {
			auto spline_node = std::static_pointer_cast<SplineNode>(node);

			bool animate = spline_node->m_animate;

			ImGui::Checkbox("Animate", &spline_node->m_animate);
			ImGui::DragFloat("Speed", &spline_node->m_speed);
			ImGui::DragFloat("Time	", &spline_node->m_time);

			// Started animating
			if (spline_node->m_animate && animate != spline_node->m_animate)
			{
				m_initial_position = scene_graph->GetActiveCamera()->m_position;
				m_initial_rotation = scene_graph->GetActiveCamera()->m_rotation_radians;
			}

			// Stopped animating
			if (!spline_node->m_animate && animate != spline_node->m_animate)
			{
				scene_graph->GetActiveCamera()->SetPosition(m_initial_position);
				scene_graph->GetActiveCamera()->SetRotation(m_initial_rotation);
			}

			if (ImGui::Button("Save Spline"))
			{
				auto result = SaveDialog();

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
				auto result = LoadDialog();

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
				ControlPoint cp;
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

					ImGui::TreePop();
				}
			}

			spline_node->UpdateNaturalSpline();
		}
	};
}

SplineNode::~SplineNode()
{
	if (m_spline) delete m_spline;
}

void SplineNode::UpdateSplineNode(float delta, std::shared_ptr<wr::Node> node)
{
	if (!m_spline || !m_quat_spline || !m_animate) return; // Don't try to run this code if we don't have a spline.

	auto num_points = m_control_points.size();

	m_time += delta * m_speed;
	m_time = std::fmod(m_time, m_spline->getMaxT());

	auto lerp_delta = m_time / m_spline->getMaxT();
#ifdef LOOPING
	auto segment_i = lerp_delta * ((float)num_points);
	auto alpha = (lerp_delta * ((float)num_points)) - floor(segment_i);
#else
	auto segment_i = lerp_delta * ((float)num_points - 1.f);
	auto alpha = (lerp_delta * ((float)num_points - 1.f)) - floor(segment_i);
#endif

	float backback = std::fmod((std::fmod(floor(segment_i), num_points) + num_points), num_points);
	auto back_point = m_control_points[backback];
	auto prev_point = m_control_points[floor(segment_i)];
	auto next_point = m_control_points[fmod(ceil(segment_i), num_points)];
	auto end_point = m_control_points[fmod(ceil(segment_i+1), num_points)];

	auto rot_a = DirectX::XMQuaternionRotationRollPitchYawFromVector(prev_point.m_rotation);
	auto rot_b = DirectX::XMQuaternionRotationRollPitchYawFromVector(next_point.m_rotation);
	auto interp = DirectX::XMQuaternionSlerp(rot_a, rot_b, alpha);

	auto new_pos = m_spline->getPosition(m_time);
	auto new_rot = m_quat_spline->getPosition(m_time);

	LOG("{} -- {}", floor(segment_i), fmod(ceil(segment_i), num_points));

	node->SetPosition({ new_pos[0], new_pos[1], new_pos[2] });
	node->SetRotationQuat(interp);
	node->SetRotationQuat({ new_rot[0], new_rot[1], new_rot[2], new_rot[3] });
}

void SplineNode::UpdateNaturalSpline()
{
	if (m_spline)
	{
		delete m_spline;
		m_spline = nullptr;
	}

	if (m_quat_spline)
	{
		delete m_quat_spline;
		m_quat_spline = nullptr;
	}

	if (m_control_points.size() < 3)
	{
		return;
	}

	{
		std::vector<Vector<3>> m_spline_positions;

		for (auto const& cp : m_control_points)
		{
			m_spline_positions.push_back(Vector<3>({ cp.m_position.m128_f32[0], cp.m_position.m128_f32[1], cp.m_position.m128_f32[2] }));
		}

#ifdef LOOPING
		m_spline = new LoopingNaturalSpline<Vector<3>>(m_spline_positions);
#else
		m_spline = new NaturalSpline<Vector<3>>(m_spline_positions);
#endif
	}

	{
		std::vector<Vector<4>> m_spline_rotations;

		for (auto const& cp : m_control_points)
		{
			auto quat = DirectX::XMQuaternionRotationRollPitchYawFromVector(cp.m_rotation);
			m_spline_rotations.push_back(Vector<4>({ quat.m128_f32[0], quat.m128_f32[1], quat.m128_f32[2], quat.m128_f32[3] }));
		}
#ifdef LOOPING
		m_quat_spline = new LoopingNaturalSpline<Vector<4>>(m_spline_positions);
#else
		m_quat_spline = new NaturalSpline<Vector<4>>(m_spline_rotations);
#endif
	}
}

std::optional<std::string> SplineNode::LoadDialog()
{
	char buffer[MAX_PATH];

	OPENFILENAME dialog_args;
	ZeroMemory(&dialog_args, sizeof(dialog_args));
	dialog_args.lStructSize = sizeof(OPENFILENAME);
	dialog_args.hwndOwner = NULL;
	dialog_args.lpstrFile = buffer;
	dialog_args.lpstrFile[0] = '\0';
	dialog_args.nMaxFile = sizeof(buffer);
	dialog_args.lpstrFilter = "All\0*.*\0Spline\0*.SPL\0";
	dialog_args.nFilterIndex = 1;
	dialog_args.lpstrTitle = "Load Spline";
	dialog_args.lpstrFileTitle = NULL;
	dialog_args.nMaxFileTitle = 0;
	dialog_args.lpstrInitialDir = NULL;
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
	dialog_args.hwndOwner = NULL;
	dialog_args.lpstrFile = buffer;
	//dialog_args.lpstrFile[0] = '\0';
	dialog_args.nMaxFile = sizeof(buffer);
	dialog_args.lpstrFilter = "Spline\0*.SPL\0";
	dialog_args.nFilterIndex = 1;
	dialog_args.lpstrFileTitle = NULL;
	dialog_args.lpstrTitle = "Save Spline";
	dialog_args.nMaxFileTitle = 0;
	dialog_args.lpstrInitialDir = NULL;
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
		ControlPoint cp;
		cp.m_position = { numbers[0], numbers[1], numbers[2] };
		cp.m_rotation = { numbers[3], numbers[4], numbers[5] };
		m_control_points.push_back(cp);
		 
		numbers.erase(numbers.begin(), numbers.begin() + 6);
	}

	file.close();
}

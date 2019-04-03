#include "spline_node.hpp"

#include <imgui_tools.hpp>
#include <scene_graph/camera_node.hpp>
#include <scene_graph/scene_graph.hpp>

SplineNode::SplineNode() : Node(typeid(SplineNode)), m_animate(false), m_speed(1), m_time(0), m_spline(nullptr)
{
	wr::imgui::window::SceneGraphEditorDetails::sg_editor_type_names[typeid(SplineNode)] =
	{
		[](std::shared_ptr<Node> node) -> std::string {
			return "Spline Node";
		}
	};

	wr::imgui::window::SceneGraphEditorDetails::sg_editor_type_inspect[typeid(SplineNode)] =
	{
		[&](std::shared_ptr<Node> node, wr::SceneGraph* scene_graph) {
			bool animate = m_animate;

			ImGui::Checkbox("Animate", &m_animate);
			ImGui::DragFloat("Speed", &m_speed);
			ImGui::DragFloat("Time	", &m_time);

			// Started animating
			if (m_animate && animate != m_animate)
			{
				m_initial_position = scene_graph->GetActiveCamera()->m_position;
				m_initial_rotation = scene_graph->GetActiveCamera()->m_rotation_radians;
			}

			// Stopped animating
			if (!m_animate && animate != m_animate)
			{
				scene_graph->GetActiveCamera()->SetPosition(m_initial_position);
				scene_graph->GetActiveCamera()->SetRotation(m_initial_rotation);
			}

			if (ImGui::Button("Add Control Point"))
			{
				ControlPoint cp;
				cp.m_position = {0, 0, 0};
				cp.m_rotation = { 0, 0, 0};

				m_control_points.push_back(cp);
			}

			ImGui::Separator();

			for (std::size_t i = 0; i < m_control_points.size(); i++)
			{
				auto i_str = std::to_string(i);

				if (ImGui::TreeNode(("Control Point " + i_str).c_str()))
				{
					auto& cp = m_control_points[i];
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

			UpdateNaturalSpline();
		}
	};

	m_control_points.push_back({ { 0, 0.5, 0.8 }, { 0, 0, 0 } });
	m_control_points.push_back({ { 0.8, 0.5, 0 }, { 0, 90_deg, 0 } });
	m_control_points.push_back({ { 0, 0.5, -0.8 }, { 0, 180_deg, 0 } });
	m_control_points.push_back({ { -0.8, 0.5, 0 }, { 0, -90_deg, 0 } });
}

SplineNode::~SplineNode()
{
	delete m_spline;
}

#define LOOPING
void SplineNode::UpdateSplineNode(float delta, std::shared_ptr<wr::Node> node)
{
	if (!m_spline || !m_animate) return; // Don't try to run this code if we don't have a spline.

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

	auto prev_point = m_control_points[floor(segment_i)];
	auto next_point = m_control_points[fmod(ceil(segment_i), num_points)];

	auto rot_a = DirectX::XMQuaternionRotationRollPitchYawFromVector(prev_point.m_rotation);
	auto rot_b = DirectX::XMQuaternionRotationRollPitchYawFromVector(next_point.m_rotation);
	auto interp = DirectX::XMQuaternionSlerp(rot_a, rot_b, alpha);

	auto new_pos = m_spline->getPosition(m_time);

	LOG("{} -- {}", floor(segment_i), fmod(ceil(segment_i), num_points));

	node->SetPosition({ new_pos[0], new_pos[1], new_pos[2] });
	node->SetRotationQuat(interp);
}

void SplineNode::UpdateNaturalSpline()
{
	if (m_control_points.size() < 3)
	{
		if (m_spline)
		{
			delete m_spline;
			m_spline = nullptr;
		}
		return;
	}

	std::vector<Vector<3>> m_spline_positions;

	for (auto const& cp : m_control_points)
	{
		m_spline_positions.push_back(Vector<3>({ cp.m_position.m128_f32[0], cp.m_position.m128_f32[1], cp.m_position.m128_f32[2] }));
	}

	m_spline = new LoopingNaturalSpline<Vector<3>>(m_spline_positions);
}

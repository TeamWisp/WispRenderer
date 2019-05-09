#pragma once

#include <scene_graph/node.hpp>
#include "spline_library/splines/natural_spline.h"
#include "spline_library/vector.h"
#include <optional>

class SplineNode : public wr::Node
{
	struct ControlPoint
	{
		DirectX::XMVECTOR m_position;
		DirectX::XMVECTOR m_rotation;
	};

public:
	explicit SplineNode(std::string name, bool looping = false);
	~SplineNode();

	void UpdateSplineNode(float delta, std::shared_ptr<wr::Node> node);

private:
	void UpdateNaturalSpline();

	static std::optional<std::string> LoadDialog();
	static std::optional<std::string> SaveDialog();
	void SaveSplineToFile(std::string const & path);
	void LoadSplineFromFile(std::string const & path);

	bool m_animate;

	void* m_spline;
	void* m_quat_spline;

	std::vector<ControlPoint> m_control_points;
	float m_speed;
	float m_time;

	bool m_looping;

	std::string m_name;
};
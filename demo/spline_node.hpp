#pragma once

#include <scene_graph/node.hpp>
#include "spline_library/splines/natural_spline.h"
#include "spline_library/vector.h"

class SplineNode : public wr::Node
{
	struct ControlPoint
	{
		DirectX::XMVECTOR m_position;
		DirectX::XMVECTOR m_rotation;
	};

public:
	SplineNode();
	~SplineNode();

	void UpdateSplineNode(float delta, std::shared_ptr<wr::Node> node);

private:
	void UpdateNaturalSpline();

	bool m_animate;

	DirectX::XMVECTOR m_initial_position;
	DirectX::XMVECTOR m_initial_rotation;

#ifdef LOOPING
	LoopingNaturalSpline<Vector<3>>* m_spline;
	LoopingNaturalSpline<Vector<4>>* m_quat_spline;
#else
	NaturalSpline<Vector<3>>* m_spline;
	NaturalSpline<Vector<4>>* m_quat_spline;
#endif
	std::vector<ControlPoint> m_control_points;
	float m_speed;
	float m_time;
};
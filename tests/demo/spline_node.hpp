/*!
 * Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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
	explicit SplineNode(std::string name, bool looping = false, bool play_once = false);
	~SplineNode();

	void UpdateSplineNode(float delta, std::shared_ptr<wr::Node> node);
	void SaveSplineToFile(std::string const & path);
	void LoadSplineFromFile(std::string const & path);
	void UpdateNaturalSpline();

	bool m_animate;

	static std::optional<std::string> LoadDialog();
	static std::optional<std::string> SaveDialog();


	void* m_spline;
	void* m_quat_spline;

	std::vector<ControlPoint> m_control_points;
	float m_speed;
	float m_time;

	bool m_looping;
	bool m_play_once;

	std::string m_name;
};
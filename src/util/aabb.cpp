// Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "aabb.hpp"

namespace wr
{

	Box::Box() : 
		m_data
		{ 
			{
				std::numeric_limits<float>::max(),
				std::numeric_limits<float>::max(),
				std::numeric_limits<float>::max()
			},
			{
				-std::numeric_limits<float>::max(),
				-std::numeric_limits<float>::max(),
				-std::numeric_limits<float>::max()
			},
			{
				std::numeric_limits<float>::max(),
				std::numeric_limits<float>::max(),
				std::numeric_limits<float>::max()
			},
			{
				-std::numeric_limits<float>::max(),
				-std::numeric_limits<float>::max(),
				-std::numeric_limits<float>::max()
			},
			{
				std::numeric_limits<float>::max(),
				std::numeric_limits<float>::max(),
				std::numeric_limits<float>::max()
			},
			{
				-std::numeric_limits<float>::max(),
				-std::numeric_limits<float>::max(),
				-std::numeric_limits<float>::max()
			} 
		}
	{

	}

	Box::Box(DirectX::XMVECTOR(&corners)[6])
	{
		memcpy(m_data, corners, sizeof(corners));
	}

	DirectX::XMVECTOR& Box::operator[](size_t i)
	{
		return m_data[i];
	}

	DirectX::XMVECTOR& AABB::operator[](size_t i)
	{
		return m_data[i];
	}

	void AABB::Expand(DirectX::XMVECTOR pos)
	{
		m_min =
		{
			std::min(pos.m128_f32[0], *m_minf),
			std::min(pos.m128_f32[1], m_minf[1]),
			std::min(pos.m128_f32[2], m_minf[2]),
			1
		};

		m_max =
		{
			std::max(pos.m128_f32[0], *m_maxf),
			std::max(pos.m128_f32[1], m_maxf[1]),
			std::max(pos.m128_f32[2], m_maxf[2]),
			1
		};
	}

	AABB::AABB() : 
		m_data
		{
			{
				std::numeric_limits<float>::max(),
				std::numeric_limits<float>::max(),
				std::numeric_limits<float>::max(),
				1
			},
			{
				-std::numeric_limits<float>::max(),
				-std::numeric_limits<float>::max(),
				-std::numeric_limits<float>::max(),
				1
			}
		}
	{

	}

	AABB::AABB(DirectX::XMVECTOR min, DirectX::XMVECTOR max) : m_data{ min, max }
	{
	}

	AABB AABB::FromTransform(Box box, DirectX::XMMATRIX transform)
	{
		AABB aabb;

		//Transform all coords from model to world space
		//Pick the min/max bounds

		for (DirectX::XMVECTOR& vec : box.m_data)
		{
			DirectX::XMVECTOR tvec = DirectX::XMVector4Transform(vec, transform);
			aabb.Expand(tvec);
		}

		return aabb;
	}

	void Box::ExpandFromVector(DirectX::XMVECTOR pos)
	{

		if (pos.m128_f32[0] < m_corners.m_xmin.m128_f32[0])
		{
			m_corners.m_xmin = pos;
		}

		if (pos.m128_f32[0] > m_corners.m_xmax.m128_f32[0])
		{
			m_corners.m_xmax = pos;
		}

		if (pos.m128_f32[1] < m_corners.m_ymin.m128_f32[1])
		{
			m_corners.m_ymin = pos;
		}

		if (pos.m128_f32[1] > m_corners.m_ymax.m128_f32[1])
		{
			m_corners.m_ymax = pos;
		}

		if (pos.m128_f32[2] < m_corners.m_zmin.m128_f32[2])
		{
			m_corners.m_zmin = pos;
		}

		if (pos.m128_f32[2] > m_corners.m_zmax.m128_f32[2])
		{
			m_corners.m_zmax = pos;
		}
	}

	void Box::Expand(float(&pos)[3])
	{
		ExpandFromVector(DirectX::XMVECTOR{ pos[0], pos[1], pos[2], 1 });
	}

	Sphere::Sphere(): m_sphere { 
		std::numeric_limits<float>::max(), 
		std::numeric_limits<float>::max(), 
		std::numeric_limits<float>::max(), 
		-std::numeric_limits<float>::max()
	}{ }

	Sphere::Sphere(DirectX::XMVECTOR center, float radius): m_sphere {
		center.m128_f32[0],
		center.m128_f32[1],
		center.m128_f32[2],
		radius
	}{ }

	bool AABB::InFrustum(const std::array<DirectX::XMVECTOR, 6>& planes) const
	{
		for (const DirectX::XMVECTOR& plane : planes)
		{
			/* Get point of AABB that's into the plane the most */

			DirectX::XMVECTOR axis_vert = {
				*m_data[*plane.m128_f32 >= 0].m128_f32,
				m_data[plane.m128_f32[1] >= 0].m128_f32[1],
				m_data[plane.m128_f32[2] >= 0].m128_f32[2]
			};

			/* Check if it's outside */

			if (*DirectX::XMVector3Dot(plane, axis_vert).m128_f32 + plane.m128_f32[3] < 0)
				return false;

		}

		return true;

	}

	bool AABB::Contains(const Sphere& sphere) const
	{
		float square_dist = 0;

		for(size_t i = 0; i < 3; ++i)
		{
			if(sphere.m_data[i] < m_minf[i])
			{
				square_dist += pow(sphere.m_data[i] - m_minf[i], 2);
			}
			else if(sphere.m_data[i] > m_maxf[i])
			{
				square_dist += pow(sphere.m_data[i] - m_maxf[i], 2);
			}
		}

		const float r_squared = sphere.m_radius * sphere.m_radius;
		return square_dist <= r_squared;
	}

}
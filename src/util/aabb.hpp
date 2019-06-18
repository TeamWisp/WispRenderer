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
#include <DirectXMath.h>
#include <limits>
#include <algorithm>
#include <array>

namespace wr
{
	struct Box
	{
		struct Corners
		{
			DirectX::XMVECTOR m_xmin, m_xmax, m_ymin, m_ymax, m_zmin, m_zmax;
		};

		union
		{
			Corners m_corners;
			DirectX::XMVECTOR m_data[6];
		};

		//Max bounds on each corner
		Box();

		Box(DirectX::XMVECTOR (&corners)[6]);

		DirectX::XMVECTOR &operator[](size_t i);

		//Expand bounds using position
		void ExpandFromVector(DirectX::XMVECTOR pos);

		void Expand(float(&pos)[3]);

	};

	struct Sphere 
	{
		
		union
		{
			DirectX::XMVECTOR m_sphere;

			struct
			{
				DirectX::XMFLOAT3 m_center;
				float m_radius;
			};

			struct {
				float m_data[4];
			};

		};

		Sphere();
		Sphere(DirectX::XMVECTOR center, float radius);

	};

	struct AABB
	{
		union
		{

			DirectX::XMVECTOR m_data[2];

			struct
			{
				DirectX::XMVECTOR m_min, m_max;
			};

			struct {

				float m_minf[3];
				float m_pad0;

				float m_maxf[3];
				float m_pad1;

			};

		};

		AABB();
		AABB(DirectX::XMVECTOR min, DirectX::XMVECTOR max);

		DirectX::XMVECTOR& operator[](size_t i);

		//Uses position to expand the AABB
		void Expand(DirectX::XMVECTOR pos);

		//Check if the frustum planes intersect with the AABB
		bool InFrustum(const std::array<DirectX::XMVECTOR, 6>& planes) const;

		//Check if the sphere intersects with the AABB
		bool Contains(const Sphere& sphere) const;

		//Generates AABB from transform and box
		static AABB FromTransform(Box box, DirectX::XMMATRIX transform);

	};

}
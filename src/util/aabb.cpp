#include "aabb.hpp"

namespace wr
{

	Box::Box() : 
		m_corners
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
		memcpy(m_corners, corners, sizeof(corners));
	}

	DirectX::XMVECTOR& Box::operator[](size_t i)
	{
		return m_corners[i];
	}

	DirectX::XMVECTOR& AABB::operator[](size_t i)
	{
		return m_bounds[i];
	}

	void AABB::Expand(DirectX::XMVECTOR pos)
	{
		m_min =
		{
			std::min(pos.m128_f32[0], *m_min.m128_f32),
			std::min(pos.m128_f32[1], m_min.m128_f32[1]),
			std::min(pos.m128_f32[2], m_min.m128_f32[2]),
			1
		};

		m_max =
		{
			std::max(pos.m128_f32[0], *m_max.m128_f32),
			std::max(pos.m128_f32[1], m_max.m128_f32[1]),
			std::max(pos.m128_f32[2], m_max.m128_f32[2]),
			1
		};
	}

	AABB::AABB() : 

		m_min
		{
			std::numeric_limits<float>::max(),
			std::numeric_limits<float>::max(),
			std::numeric_limits<float>::max()
		},

		m_max
		{
			-std::numeric_limits<float>::max(),
			-std::numeric_limits<float>::max(),
			-std::numeric_limits<float>::max()
		}
	{

	}

	AABB::AABB(DirectX::XMVECTOR min, DirectX::XMVECTOR max) : m_min(m_min), m_max(m_max)
	{
	}

	AABB AABB::FromTransform(Box box, DirectX::XMMATRIX transform)
	{
		AABB aabb;

		//Transform all coords from model to world space
		//Pick the min/max bounds

		for (DirectX::XMVECTOR& vec : box.m_corners)
		{
			DirectX::XMVECTOR tvec = DirectX::XMVector4Transform(vec, transform);
			aabb.Expand(tvec);
		}

		return aabb;
	}

	void Box::ExpandFromVector(DirectX::XMVECTOR pos)
	{
		//-X
		if (pos.m128_f32[0] < m_corners[0].m128_f32[0])
		{
			m_corners[0] = pos;
		}

		//X
		if (pos.m128_f32[0] > m_corners[1].m128_f32[0])
		{
			m_corners[1] = pos;
		}

		//-Y
		if (pos.m128_f32[1] < m_corners[2].m128_f32[1])
		{
			m_corners[2] = pos;
		}

		//Y
		if (pos.m128_f32[1] > m_corners[3].m128_f32[1])
		{
			m_corners[3] = pos;
		}

		//-Z
		if (pos.m128_f32[2] < m_corners[4].m128_f32[2])
		{
			m_corners[4] = pos;
		}

		//Z
		if (pos.m128_f32[2] > m_corners[5].m128_f32[2])
		{
			m_corners[5] = pos;
		}
	}

	void Box::Expand(float(&pos)[3])
	{
		ExpandFromVector(DirectX::XMVECTOR{ pos[0], pos[1], pos[2], 1 });
	}

	bool AABB::InFrustum(DirectX::XMVECTOR(&planes)[6])
	{
		for (DirectX::XMVECTOR& plane : planes)
		{
			/* Get point of AABB that's into the plane the most */

			DirectX::XMVECTOR axis_vert = {
				*m_bounds[*plane.m128_f32 >= 0].m128_f32,
				m_bounds[plane.m128_f32[1] >= 0].m128_f32[1],
				m_bounds[plane.m128_f32[2] >= 0].m128_f32[2]
			};

			/* Check if it's outside */

			if (*DirectX::XMVector3Dot(plane, axis_vert).m128_f32 + plane.m128_f32[3] < 0)
				return false;

		}

		return true;

	}

}
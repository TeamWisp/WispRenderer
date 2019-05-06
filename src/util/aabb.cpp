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
		m_corners.m_min =
		{
			std::min(pos.m128_f32[0], *m_corners.m_min.m128_f32),
			std::min(pos.m128_f32[1], m_corners.m_min.m128_f32[1]),
			std::min(pos.m128_f32[2], m_corners.m_min.m128_f32[2]),
			1
		};

		m_corners.m_max =
		{
			std::max(pos.m128_f32[0], *m_corners.m_max.m128_f32),
			std::max(pos.m128_f32[1], m_corners.m_max.m128_f32[1]),
			std::max(pos.m128_f32[2], m_corners.m_max.m128_f32[2]),
			1
		};
	}

	AABB::AABB() : 
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
			}
		}
	{

	}

	AABB::AABB(DirectX::XMVECTOR min, DirectX::XMVECTOR max) : m_corners{ min, max }
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

	bool AABB::InFrustum(std::array<DirectX::XMVECTOR, 6> planes)
	{
		for (DirectX::XMVECTOR& plane : planes)
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

}
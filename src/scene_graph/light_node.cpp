#include "light_node.hpp"

namespace wr
{

	LightNode::LightNode(LightType tid, DirectX::XMVECTOR col) : Node(), m_light(&m_temp)
	{
		SetType(tid);
		SetColor(col);
	}

	LightNode::LightNode(DirectX::XMVECTOR dir, DirectX::XMVECTOR col) : Node(), m_light(&m_temp)
	{
		SetDirectional(dir, col);
	}

	LightNode::LightNode(DirectX::XMVECTOR pos, float rad, DirectX::XMVECTOR col) : Node(), m_light(&m_temp)
	{
		SetPoint(pos, rad, col);
	}

	LightNode::LightNode(DirectX::XMVECTOR pos, float rad, DirectX::XMVECTOR dir, float ang, DirectX::XMVECTOR col) : Node(), m_light(&m_temp)
	{
		SetSpot(pos, rad, dir, ang, col);
	}

	LightNode::LightNode(const LightNode& old)
	{
		m_temp = old.m_temp;
		m_temp.tid &= 0x3;
		m_light = &m_temp;
	}

	LightNode& LightNode::operator=(const LightNode& old)
	{
		m_temp = old.m_temp;
		m_temp.tid &= 0x3;
		m_light = &m_temp;
		return *this;
	}

	void LightNode::SetAngle(float ang)
	{
		m_light->ang = ang / 180.f * 3.1415926535f;
		SignalChange();
	}

	void LightNode::SetRadius(float rad)
	{
		m_light->rad = rad;
		SignalChange();
	}

	void LightNode::SetType(LightType tid)
	{
		m_light->tid = (uint32_t) tid;
		SignalChange();
	}

	void LightNode::SetColor(DirectX::XMVECTOR col)
	{
		memcpy(&m_light->col, &col, 12);
		SignalChange();
	}

	void LightNode::SetDirectional(DirectX::XMVECTOR rot_deg, DirectX::XMVECTOR col) 
	{
		SetType(LightType::DIRECTIONAL);
		SetRotation(rot_deg);
		SetColor(col);
	}

	void LightNode::SetPoint(DirectX::XMVECTOR pos, float rad, DirectX::XMVECTOR col)
	{
		SetType(LightType::POINT);
		SetPosition(pos);
		SetRadius(rad);
		SetColor(col);
	}

	void LightNode::SetSpot(DirectX::XMVECTOR pos, float rad, DirectX::XMVECTOR rot_deg, float ang, DirectX::XMVECTOR col)
	{
		SetType(LightType::POINT);
		SetPosition(pos);
		SetRadius(rad);
		SetRotation(rot_deg);
		SetAngle(ang);
		SetColor(col);
	}

	void LightNode::Update(uint32_t frame_idx)
	{
		DirectX::XMVECTOR position = { m_transform.r[3].m128_f32[0], m_transform.r[3].m128_f32[1], m_transform.r[3].m128_f32[2] };
		memcpy(&m_light->pos, &position, 12);

		DirectX::XMVECTOR forward = DirectX::XMVector3Normalize(m_transform.r[2]);
		memcpy(&m_light->dir, &forward, 12);

		SignalUpdate(frame_idx);
	}

	LightType LightNode::GetType()
	{
		return (LightType)(m_light->tid & 0x3);
	}

}
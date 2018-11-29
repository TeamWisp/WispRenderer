#include "light_node.hpp"

namespace wr
{

	LightNode::LightNode(LightType tid, DirectX::XMVECTOR col) : Node()
	{
		SetType(tid);
		SetColor(col);
	}

	LightNode::LightNode(DirectX::XMVECTOR dir, DirectX::XMVECTOR col) : Node()
	{
		SetDirectional(dir, col);
	}

	LightNode::LightNode(DirectX::XMVECTOR pos, float rad, DirectX::XMVECTOR col) : Node()
	{
		SetPoint(pos, rad, col);
	}

	LightNode::LightNode(DirectX::XMVECTOR pos, float rad, DirectX::XMVECTOR dir, float ang, DirectX::XMVECTOR col) : Node()
	{
		SetSpot(pos, rad, dir, ang, col);
	}

	//Set angle in degrees
	void LightNode::SetAngle(float ang)
	{
		m_light.ang = ang / 180.f * 3.1415926535f;
		SignalChange();
	}

	//Set radius
	void LightNode::SetRadius(float rad)
	{
		m_light.rad = rad;
		SignalChange();
	}

	//Set type
	void LightNode::SetType(LightType tid)
	{
		m_light.tid = (uint32_t) tid;
		SignalChange();
	}

	//Sets position
	void LightNode::SetPosition(DirectX::XMVECTOR pos)
	{
		memcpy(&m_light.pos, &pos, 12);
		SignalChange();
	}

	//Sets direction
	void LightNode::SetDirection(DirectX::XMVECTOR dir)
	{
		dir = DirectX::XMVector3Normalize(dir);
		memcpy(&m_light.dir, &dir, 12);
		SignalChange();
	}

	//Sets color
	void LightNode::SetColor(DirectX::XMVECTOR col)
	{
		memcpy(&m_light.col, &col, 12);
		SignalChange();
	}

	//Set data for a directional light
	void LightNode::SetDirectional(DirectX::XMVECTOR dir, DirectX::XMVECTOR col) 
	{
		SetType(LightType::DIRECTIONAL);
		SetDirection(dir);
		SetColor(col);
	}

	//Set data for a point light
	void LightNode::SetPoint(DirectX::XMVECTOR pos, float rad, DirectX::XMVECTOR col)
	{
		SetType(LightType::POINT);
		SetPosition(pos);
		SetRadius(rad);
		SetColor(col);
	}

	//Set data for a spot light
	void LightNode::SetSpot(DirectX::XMVECTOR pos, float rad, DirectX::XMVECTOR dir, float ang, DirectX::XMVECTOR col)
	{
		SetType(LightType::POINT);
		SetPosition(pos);
		SetRadius(rad);
		SetDirection(dir);
		SetAngle(ang);
		SetColor(col);
	}

}
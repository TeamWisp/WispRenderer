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
#include "light_node.hpp"

namespace wr
{

	LightNode::LightNode(LightType tid, DirectX::XMVECTOR col) : Node(typeid(LightNode)), m_light(&m_temp)
	{
		SetType(tid);
		SetColor(col);
	}

	LightNode::LightNode(DirectX::XMVECTOR dir, DirectX::XMVECTOR col) : Node(typeid(LightNode)), m_light(&m_temp)
	{
		SetDirectional(dir, col);
	}

	LightNode::LightNode(DirectX::XMVECTOR pos, float rad, DirectX::XMVECTOR col) : Node(typeid(LightNode)), m_light(&m_temp)
	{
		SetPoint(pos, rad, col);
	}

	LightNode::LightNode(DirectX::XMVECTOR pos, float rad, DirectX::XMVECTOR dir, float ang, DirectX::XMVECTOR col) : Node(typeid(LightNode)), m_light(&m_temp)
	{
		SetSpot(pos, rad, dir, ang, col);
	}

	LightNode::LightNode(const LightNode& old) : Node(typeid(LightNode))
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
		m_light->ang = ang;
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

	void LightNode::SetDirectional(DirectX::XMVECTOR rot, DirectX::XMVECTOR col) 
	{
		SetType(LightType::DIRECTIONAL);
		SetRotation(rot);
		SetColor(col);
	}

	void LightNode::SetPoint(DirectX::XMVECTOR pos, float rad, DirectX::XMVECTOR col)
	{
		SetType(LightType::POINT);
		SetPosition(pos);
		SetRadius(rad);
		SetColor(col);
	}

	void LightNode::SetSpot(DirectX::XMVECTOR pos, float rad, DirectX::XMVECTOR rot, float ang, DirectX::XMVECTOR col)
	{
		SetType(LightType::SPOT);
		SetPosition(pos);
		SetRadius(rad);
		SetRotation(rot);
		SetAngle(ang);
		SetColor(col);
	}

	void LightNode::SetLightSize(float size_in_deg)
	{
		m_light->light_size = DirectX::XMConvertToRadians(size_in_deg);
		SignalChange();
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
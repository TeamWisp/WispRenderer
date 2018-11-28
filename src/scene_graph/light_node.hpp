#pragma once

#include "scene_graph.hpp"

namespace wr
{

	struct LightNode : Node
	{
		LightNode(LightType tid, DirectX::XMVECTOR col = { 1, 1, 1 });
		LightNode(DirectX::XMVECTOR dir, DirectX::XMVECTOR col = { 1, 1, 1 });
		LightNode(DirectX::XMVECTOR pos, float rad, DirectX::XMVECTOR col = { 1, 1, 1 });
		LightNode(DirectX::XMVECTOR pos, float rad, DirectX::XMVECTOR dir, float ang, DirectX::XMVECTOR col = { 1, 1, 1 });

		//Set angle in degrees
		void SetAngle(float ang);

		//Set radius
		void SetRadius(float rad);

		//Set type
		void SetType(LightType tid);

		//Sets position
		void SetPosition(DirectX::XMVECTOR pos);

		//Sets direction
		void SetDirection(DirectX::XMVECTOR dir);

		//Sets color
		void SetColor(DirectX::XMVECTOR col);

		//Set data for a directional light
		void SetDirectional(DirectX::XMVECTOR dir, DirectX::XMVECTOR col = { 1, 1, 1 });

		//Set data for a point light
		void SetPoint(DirectX::XMVECTOR pos, float rad, DirectX::XMVECTOR col = { 1, 1, 1 });

		//Set data for a spot light
		void SetSpot(DirectX::XMVECTOR pos, float rad, DirectX::XMVECTOR dir, float ang, DirectX::XMVECTOR col = { 1, 1, 1 });

		//Physical data
		Light m_light;

	};

} /* wr */
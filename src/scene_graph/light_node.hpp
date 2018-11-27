#pragma once

#include "scene_graph.hpp"

namespace wr
{

	enum class LightType : uint32_t
	{
		POINT, DIRECTIONAL, SPOT, FREE /* MAX LighType value; but unused */
	};

	struct Light
	{
		DirectX::XMFLOAT3 pos = { 0, 0, 0 };			//Position in world space for spot & point
		float rad = 5.f;								//Radius for point, height for spot

		DirectX::XMFLOAT3 col = { 1, 1, 1 };			//Color (and strength)
		uint32_t tid = (uint32_t)LightType::POINT;		//Type id; LightType::x

		DirectX::XMFLOAT3 dir = { 0, 0, 1 };			//Direction for spot & directional
		float ang = 40.f / 180.f * 3.1415926535f;		//Angle for spot; in radians
	};

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
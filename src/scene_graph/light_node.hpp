#pragma once

#include "node.hpp"

#include "../platform_independend_structs.hpp"

namespace wr
{

	struct LightNode : Node
	{
		explicit LightNode(LightType tid, DirectX::XMVECTOR col = { 1, 1, 1 });
		explicit LightNode(DirectX::XMVECTOR rot, DirectX::XMVECTOR col = { 1, 1, 1 });
		LightNode(DirectX::XMVECTOR pos, float rad, DirectX::XMVECTOR col = { 1, 1, 1 });
		LightNode(DirectX::XMVECTOR pos, float rad, DirectX::XMVECTOR rot, float ang, DirectX::XMVECTOR col = { 1, 1, 1 });

		LightNode(const LightNode& old);
		LightNode& operator=(const LightNode& old);

		//! Set angle
		void SetAngle(float ang);

		//! Set radius
		void SetRadius(float rad);

		//! Set type
		void SetType(LightType tid);

		//! Sets color
		void SetColor(DirectX::XMVECTOR col);

		//! Set data for a directional light
		void SetDirectional(DirectX::XMVECTOR rot, DirectX::XMVECTOR col = { 1, 1, 1 });

		//! Set data for a point light
		void SetPoint(DirectX::XMVECTOR pos, float rad, DirectX::XMVECTOR col = { 1, 1, 1 });

		//! Set data for a spot light
		void SetSpot(DirectX::XMVECTOR pos, float rad, DirectX::XMVECTOR rot, float ang, DirectX::XMVECTOR col = { 1, 1, 1 });

		//! Set data for light physical size
		void SetLightSize(float size);

		//! Update
		void Update(uint32_t frame_idx);

		//! Helper for getting the LightType (doesn't include light count for the first light)
		LightType GetType();

		//! Allocated data (either temp or array data)
		Light* m_light;

		//! Physical data
		Light m_temp;

	};

} /* wr */
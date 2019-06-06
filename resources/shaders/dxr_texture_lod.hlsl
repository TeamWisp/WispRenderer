#ifndef __DXR_TEXTURE_LOD_HLSL__
#define __DXR_TEXTURE_LOD_HLSL__

//Definition for RayCone
#include "dxr_structs.hlsl" 

// numbers prefixed with Cha mean its chapter x.x
// numbers prefixed with Fig means its figure x.x

// pa Fig 20.5
float ComputeTriangleArea(float3 P0, float3 P1, float3 P2)
{
	return length(cross((P1 - P0), (P2 - P0)));
}

// ta Fig 20.4
float ComputeTextureCoordsArea(float2 UV0, float2 UV1, float2 UV2, Texture2D T)
{
	float w, h;
	T.GetDimensions(w, h);

	return abs((UV1.x - UV0.x) * (UV2.y - UV0.y) - (UV2.x - UV0.x) * (UV1.y - UV0.y));           // Removed with and height to get mipmapping working in my code.
	return w * h * abs((UV1.x - UV0.x) * (UV2.y - UV0.y) - (UV2.x - UV0.x) * (UV1.y - UV0.y));   // Proper formula from Fig 20.4
}

// Ch 20.6
float GetTriangleLODConstant(float3 P0, float3 P1, float3 P2, float2 UV0, float2 UV1, float2 UV2, Texture2D T)
{
	float P_a = ComputeTriangleArea(P0, P1, P2);
	float T_a = ComputeTextureCoordsArea(UV0, UV1, UV2, T);
	return 0.5 * log2(T_a / P_a);
}

// Ch 20.6
float ComputeTextureLOD(RayCone cone, float3 V, float3 N, float3 P0, float3 P1, float3 P2, float2 UV0, float2 UV1, float2 UV2, Texture2D T)
{
	float w, h;
	T.GetDimensions(w, h);

	float lambda = GetTriangleLODConstant(P0, P1, P2, UV0, UV1, UV2, T);
	lambda += log2(abs(cone.width));
	lambda += 0.5 * log2(w * h);
	lambda -= log2(abs(dot(V, N)));
	return lambda;
}

// Fig 20.30
float PixelSpreadAngle(float vertical_fov, float output_height)
{
	return atan((2.f * tan(vertical_fov / 2.f)) / output_height);
}

float3 dumb_ddx(Texture2D t, float3 v)
{
	float2 pixel = DispatchRaysIndex().xy;

	float3 top_left = t[float2(pixel.x, pixel.y)].xyz;
	float3 top_right = t[float2(pixel.x+1, pixel.y)].xyz;
	float3 bottom_left = t[float2(pixel.x, pixel.y+1)].xyz;
	float3 bottom_right = t[float2(pixel.x+1, pixel.y+1)].xyz;

	float3 v1 = top_right - top_left;
	
	return v1;
}

float3 dumb_ddy(Texture2D t, float3 v)
{
	float2 pixel = DispatchRaysIndex().xy;

	float3 top_left = t[float2(pixel.x, pixel.y)].xyz;
	float3 top_right = t[float2(pixel.x+1, pixel.y)].xyz;
	float3 bottom_left = t[float2(pixel.x, pixel.y+1)].xyz;
	float3 bottom_right = t[float2(pixel.x+1, pixel.y+1)].xyz;

	float3 v2 = bottom_left - top_left;
	
	return v2;
}

float3 dumb_ddx_depth(Texture2D t, float4x4 inv_vp, float3 v)
{
	float2 uv = float2(DispatchRaysIndex().xy) / float2(DispatchRaysDimensions().xy - 1);
	uv.y = 1.f - uv.y;

	float2 pixel = DispatchRaysIndex().xy;

	float3 top_left = t[float2(pixel.x, pixel.y)].xyz;
	float3 top_right = t[float2(pixel.x+1, pixel.y)].xyz;
	float3 bottom_left = t[float2(pixel.x, pixel.y+1)].xyz;
	float3 bottom_right = t[float2(pixel.x+1, pixel.y+1)].xyz;

	// Get world space position
	const float4 ndc = float4(uv * 2.0 - 1.0, top_left.x, 1.0);
	float4 wpos = mul(inv_vp, ndc);
	float3 retval = (wpos.xyz / wpos.w).xyz;

	const float4 _ndc = float4(uv * 2.0 - 1.0, top_right.x, 1.0);
	float4 _wpos = mul(inv_vp, _ndc);
	float3 _retval = (_wpos.xyz / _wpos.w).xyz;

	return _retval - retval;
}

float3 dumb_ddy_depth(Texture2D t, float4x4 inv_vp, float3 v)
{
	float2 uv = float2(DispatchRaysIndex().xy) / float2(DispatchRaysDimensions().xy - 1);
	uv.y = 1.f - uv.y;

	float2 pixel = DispatchRaysIndex().xy;

	float3 top_left = t[float2(pixel.x, pixel.y)].xyz;
	float3 top_right = t[float2(pixel.x+1, pixel.y)].xyz;
	float3 bottom_left = t[float2(pixel.x, pixel.y+1)].xyz;
	float3 bottom_right = t[float2(pixel.x+1, pixel.y+1)].xyz;

	// Get world space position
	const float4 ndc = float4(uv * 2.0 - 1.0, bottom_left.x, 1.0);
	float4 wpos = mul(inv_vp, ndc);
	float3 retval = (wpos.xyz / wpos.w).xyz;

	const float4 _ndc = float4(uv * 2.0 - 1.0, top_right.x, 1.0);
	float4 _wpos = mul(inv_vp, _ndc);
	float3 _retval = (_wpos.xyz / _wpos.w).xyz;

	return retval - _retval;
}

float3 dumb_ddx_depth2(Texture2D t, float4x4 inv_vp, float3 v)
{
	float2 uv = float2(DispatchRaysIndex().xy) / float2(DispatchRaysDimensions().xy - 1);
	uv.y = 1.f - uv.y;

	float2 pixel = DispatchRaysIndex().xy;

	float3 top_left = t[float2(pixel.x, pixel.y)].xyz;
	float3 top_right = t[float2(pixel.x+1, pixel.y)].xyz;
	float3 bottom_left = t[float2(pixel.x, pixel.y+1)].xyz;
	float3 bottom_right = t[float2(pixel.x+1, pixel.y+1)].xyz;

	float3 v1 = top_right - top_left;

	// Get world space position
	const float4 ndc = float4(uv * 2.0 - 1.0, v1.x, 1.0);
	float4 wpos = mul(inv_vp, ndc);
	float3 retval = (wpos.xyz / wpos.w).xyz;

	return retval;
}

float3 dumb_ddy_depth2(Texture2D t, float4x4 inv_vp, float3 v)
{
	float2 uv = float2(DispatchRaysIndex().xy) / float2(DispatchRaysDimensions().xy - 1);
	uv.y = 1.f - uv.y;

	float2 pixel = DispatchRaysIndex().xy;

	float3 top_left = t[float2(pixel.x, pixel.y)].xyz;
	float3 top_right = t[float2(pixel.x+1, pixel.y)].xyz;
	float3 bottom_left = t[float2(pixel.x, pixel.y+1)].xyz;
	float3 bottom_right = t[float2(pixel.x+1, pixel.y+1)].xyz;

	float3 v2 = bottom_left - top_right;

	// Get world space position
	const float4 ndc = float4(uv * 2.0 - 1.0, v2.x, 1.0);
	float4 wpos = mul(inv_vp, ndc);
	float3 retval = (wpos.xyz / wpos.w).xyz;

	return retval;
}

// Fig 20.23
float ComputeSurfaceSpreadAngle(Texture2D g_P, Texture2D g_N, /*Texture2D g_DY, Texture2D g_DY2,*/ float4x4 inv_vp, float3 P, float3 N)
{
	float2 pixel = DispatchRaysIndex().xy;

	float3 aPx = dumb_ddx_depth2(g_P, inv_vp, P);
	float3 aPy = dumb_ddy_depth2(g_P, inv_vp, P);

	//float3 aPx = dumb_ddx(g_P, P);
	//float3 aPy = dumb_ddy(g_P, P);

	float3 aNx = dumb_ddx(g_N, N);
	float3 aNy = dumb_ddy(g_N, N);

	float k1 = 1;
	float k2 = 0;
	
	float s = sign(dot(aPx, aNx) + dot(aPy, aNy));
	// s = g_DY2[pixel].x; // Sign calculated in the deferred main shader.
	// s = 1 // Sets the sign to convex only. can fix a lot of issues.

	return 2.f * k1 * s * sqrt(dot(aNx, aNx) + dot(aNy, aNy)) + k2;
}

// Ch 20.6
RayCone Propagate(RayCone cone, float surface_spread_angle, float hit_dist)
{
	RayCone new_cone;
 	new_cone.width = cone.spread_angle * hit_dist + cone.width;
	new_cone.spread_angle = cone.spread_angle + surface_spread_angle;

	return new_cone;
}

// Ch 20.6
RayCone ComputeRayConeFromGBuffer(SurfaceHit hit, float vertical_fov, float height)
{
	RayCone cone;
	cone.width = 0;
	cone.spread_angle = PixelSpreadAngle(vertical_fov, height);

	return Propagate(cone, hit.surface_spread_angle, hit.dist);
}

#endif //__DXR_TEXTURE_LOD_HLSL__
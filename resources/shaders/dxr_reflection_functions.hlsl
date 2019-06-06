#ifndef __DXR_REFLECTION_FUNCTIONS_HLSL__
#define __DXR_REFLECTION_FUNCTIONS_HLSL__

#include "dxr_global.hlsl"
#include "dxr_structs.hlsl"

// Definitions for: 
// - Vertex, Material, Offset
// - Ray, RayCone, ReflectionHitInfo
#include "dxr_structs.hlsl"

// Definitions for: 
// - HitWorldPosition, Load3x32BitIndices, unpack_position, HitAttribute
#include "dxr_functions.hlsl"

#include "pbr_util.hlsl"

float3 TraceReflectionRay(float3 origin, float3 norm, float3 direction, uint rand_seed, uint depth, RayCone cone, inout float4 dirT)
{

	if (depth >= MAX_RECURSION)
	{
		return skybox.SampleLevel(s0, direction, 0).rgb;
	}

	origin += norm * EPSILON;

	ReflectionHitInfo payload = {origin, float3(0,0,1), rand_seed, depth, 0, cone};

	// Define a ray, consisting of origin, direction, and the min-max distance values
	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = direction;
	ray.TMin = 0;
	ray.TMax = 10000.0;

	bool nan = isnan(origin) != bool3(false, false, false) || isnan(direction) != bool3(false, false, false);
	if(nan)
	{
		return skybox.SampleLevel(s0, direction, 0).rgb;
	}

	// Trace the ray
	TraceRay(
		Scene,
		RAY_FLAG_NONE,
		//RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH
		//RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
		0xFF, // InstanceInclusionMask
		0, // RayContributionToHitGroupIndex
		1, // MultiplierForGeometryContributionToHitGroupIndex
		0, // miss shader index
		ray,
		payload);

	dirT = float4(direction, payload.hitT);
	return payload.color;
}

float4 DoReflection(float3 wpos, float3 V, float3 N, uint rand_seed, uint depth, float roughness, RayCone cone, inout float4 dirT)
{
	// Calculate ray info
	float3 reflected = reflect(-V, N);

	// Shoot an importance sampled ray

	#ifndef PERFECT_MIRROR_REFLECTIONS


		// Shoot perfect mirror ray if enabled or if it's a recursion or it's almost a perfect mirror

		if (depth > 0 || roughness < 0.05)
			return float4(TraceReflectionRay(wpos, N, reflected, rand_seed, depth, cone, dirT), 1);

		//Calculate an importance sampled ray

		nextRand(rand_seed);
		float2 xi = hammersley2d(rand_seed, 8192);
		float pdf = 0;
		float3 H = importanceSamplePdf(xi, roughness, N, pdf);
		float3 L = reflect(-V, H);

		float NdotL = max(dot(N, L), 0);

		if(NdotL >= 0)
		{
			nextRand(rand_seed);
			xi = hammersley2d(rand_seed, 8192);
			H = importanceSamplePdf(xi, roughness, N, pdf);
			L = reflect(-V, H);
			NdotL = max(dot(N, L), 0);
		}

		float3 reflection = float3(0, 0, 0);
		
		if (NdotL >= 0)
		{
			reflection = TraceReflectionRay(wpos, N, L, rand_seed, depth, cone, dirT);
		}

		return float4(reflection, pdf);

	#else
		// Shoot perfect mirror ray if enabled or if it's a recursion or it's almost a perfect mirror
		return float4(TraceReflectionRay(wpos, N, reflected, rand_seed, depth, cone, dirT), 1);
	#endif
}
#endif //__DXR_REFLECTION_FUNCTIONS_HLSL__
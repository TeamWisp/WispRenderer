#ifndef __SHADOW_FUNC__
#define __SHADOW_FUNC__

#include "rt_global.hlsl"
#include "util.hlsl"
#include "material_util.hlsl"
#include "rt_structs.hlsl"

bool TraceShadowRay(uint idx, float3 origin, float3 direction, float far, unsigned int depth)
{
#ifndef NO_SHADOWS
	if (depth >= MAX_RECURSION)
	{
		return false;
	}

	// Define a ray, consisting of origin, direction, and the min-max distance values
	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = direction;
	ray.TMin = 0;
	ray.TMax = far;

	ShadowHitInfo payload = { false, 0 };

	// Trace the ray
	TraceRay(
		Scene,
		RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
		~0, // InstanceInclusionMask
		1, // RayContributionToHitGroupIndex
		0, // MultiplierForGeometryContributionToHitGroupIndex
		1, // miss shader index is set to idx but can probably be anything.
		ray,
		payload);

	return payload.is_hit;
#else
	return false;
#endif
}

// Get shadow factor
float GetShadowFactor(float3 wpos, float3 light_dir, float t_max, uint depth, inout uint rand_seed)
{
	float shadow_factor = 0.0f;

//#define SOFT_SHADOWS
#ifdef SOFT_SHADOWS
	[unroll(MAX_SHADOW_SAMPLES)]
	for (uint i = 0; i < MAX_SHADOW_SAMPLES; ++i)
	{
		// Perhaps change randomness to not be purely random, but algorithm-random?
		float3 offset = normalize(float3(nextRand(rand_seed), nextRand(rand_seed), nextRand(rand_seed))) - 0.5;
		offset *= 0.02f;
		float3 ray_direction = normalize(light_dir + offset);

		bool shadow = TraceShadowRay(1, wpos, ray_direction, t_max, depth + 1);

		shadow_factor += lerp(1.0, 0.0, shadow);
	}

	shadow_factor /= float(MAX_SHADOW_SAMPLES);

#else /* ifdef SOFT_SHADOWS */

	bool shadow = TraceShadowRay(1, wpos, light_dir, t_max, depth);
	shadow_factor = !shadow;

#endif
	// Return shadow factor
	return shadow_factor;
}

#endif //__SHADOW_FUNC__

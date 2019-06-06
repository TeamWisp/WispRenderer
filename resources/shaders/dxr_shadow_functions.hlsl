#ifndef __DXR_SHADOWS_FUNCTIONS__
#define __DXR_SHADOWS_FUNCTIONS__

#include "dxr_global.hlsl"
#include "dxr_structs.hlsl"

bool TraceShadowRay(float3 origin, float3 direction, float far, uint calling_pass, unsigned int depth)
{
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

	uint ray_contr_idx = 1;
	uint miss_idx = 1;

	if (calling_pass == CALLINGPASS_SHADOWS)
	{
		ray_contr_idx = 0;
		miss_idx = 0;
	}


	// Trace the ray
	TraceRay(
		Scene,
		RAY_FLAG_NONE,
		~0, // InstanceInclusionMask
		ray_contr_idx, // RayContributionToHitGroupIndex
		0, // MultiplierForGeometryContributionToHitGroupIndex
		miss_idx, // miss shader index is set to idx but can probably be anything.
		ray,
		payload);

	return payload.is_hit;
}

// Get shadow factor
float GetShadowFactor(float3 wpos, float3 light_dir, float t_max, uint depth, uint calling_pass, inout uint rand_seed)
{
	float shadow_factor = 0.0f;

#ifdef SOFT_SHADOWS
	for (uint i = 0; i < MAX_SHADOW_SAMPLES; ++i)
	{
		// Perhaps change randomness to not be purely random, but algorithm-random?
		float3 offset = normalize(float3(nextRand(rand_seed), nextRand(rand_seed), nextRand(rand_seed))) - 0.5;
		// Hard-coded 0.05 is to minimalize the offset a ray gets
		// Should be determined by the area that the light is emitting from
		offset *= 0.05;
		float3 shadow_direction = normalize(light_dir + offset);

		bool shadow = TraceShadowRay(wpos, shadow_direction, t_max, calling_pass, depth);

		shadow_factor += lerp(1.0, 0.0, shadow);
	}

	shadow_factor /= float(MAX_SHADOW_SAMPLES);

#else /* ifdef SOFT_SHADOWS */

	bool shadow = TraceShadowRay(wpos, light_dir, t_max, calling_pass, depth);
	shadow_factor = !shadow;

#endif
	// Return shadow factor
	return shadow_factor;
}

#endif //__DXR_SHADOWS_FUNCTIONS__
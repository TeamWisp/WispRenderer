#ifndef __DXR_SHADOWS_FUNCTIONS__
#define __DXR_SHADOWS_FUNCTIONS__

#include "dxr_global.hlsl"
#include "dxr_structs.hlsl"

float2 TraceShadowRay(float3 origin, float3 direction, float far, uint calling_pass, unsigned int depth)
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

	ShadowHitInfo payload = { false, 0.0f };

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

	return float2(payload.is_hit, payload.ray_power);
}

// Get shadow factor
float GetShadowFactor(float3 wpos, float3 light_dir, float light_size, float t_max, uint sample_count, uint depth, uint calling_pass, inout uint rand_seed)
{
	float shadow_factor = 0.0f;

#ifdef SOFT_SHADOWS
	for (uint i = 0; i < sample_count; ++i)
	{
		//float3 offset = normalize(float3(nextRand(rand_seed), nextRand(rand_seed), nextRand(rand_seed))) - 0.5;
		float3 dir = perturbDirectionVector(rand_seed, light_dir, light_size);
		float3 ray_direction = normalize(dir);

		float2 result = TraceShadowRay(wpos, ray_direction, t_max, calling_pass, depth);
		float shadow = result.x;
		float ray_power = result.y;

		shadow_factor += lerp(1.0, ray_power, shadow);
	}

	shadow_factor /= float(sample_count);

#else //SOFT_SHADOWS

	float2 result = TraceShadowRay(wpos, light_dir, t_max, calling_pass, depth);
	float shadow = result.x;
	float ray_power = result.y;

	shadow_factor = lerp(1.0, ray_power, shadow);

#endif //SOFT_SHADOWS

	return shadow_factor;
}

#endif //__DXR_SHADOWS_FUNCTIONS__
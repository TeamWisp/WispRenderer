#ifndef __RT_SHADOWS_FUNCTIONS__
#define __RT_SHADOWS_FUNCTIONS__

#include "rt_global.hlsl"

#define RAY_CONTR_TO_HIT_INDEX 0
#define MISS_SHADER_OFFSET 0

struct ShadowHitInfo
{
	float is_hit;
	float thisvariablesomehowmakeshybridrenderingwork_killme;
};

struct Attributes { };

bool TraceShadowRay(uint ray_contr_to_hitgroup, uint miss_shader_idx, float3 origin, float3 direction, float far, unsigned int depth)
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

	// Trace the ray

	TraceRay(
		Scene,
		RAY_FLAG_NONE,
		0xFF, // InstanceInclusionMask
		ray_contr_to_hitgroup, // RayContributionToHitGroupIndex
		0, // MultiplierForGeometryContributionToHitGroupIndex
		miss_shader_idx, // miss shader index is set to idx but can probably be anything.
		ray,
		payload);

	return payload.is_hit;
}

// Get shadow factor
float GetShadowFactor(float3 wpos, float3 light_dir, float light_size, float t_max, uint depth, inout uint rand_seed)
{
	float shadow_factor = 0.0f;

	//#define SOFT_SHADOWS
#ifdef SOFT_SHADOWS

#define TEST_A
#ifdef TEST_A

	[unroll(MAX_SHADOW_SAMPLES)]
	for (uint i = 0; i < MAX_SHADOW_SAMPLES; ++i)
	{
		//float3 offset = normalize(float3(nextRand(rand_seed), nextRand(rand_seed), nextRand(rand_seed))) - 0.5;
		float3 dir = perturbDirectionVector(rand_seed, light_dir, light_size);
		float3 ray_direction = normalize(dir);

		bool shadow = TraceShadowRay(RAY_CONTR_TO_HIT_INDEX, MISS_SHADER_OFFSET, wpos, ray_direction, t_max, depth);

		shadow_factor += lerp(1.0, 0.0, shadow);
	}

#else
	[unroll(MAX_SHADOW_SAMPLES)]
	for (uint i = 0; i < MAX_SHADOW_SAMPLES; ++i)
	{
		float3 offset = normalize(rand_in_unit_sphere(rand_seed));
		offset *= light_size;

		float3 ray_direction = normalize(light_dir + offset);

		bool shadow = TraceShadowRay(RAY_CONTR_TO_HIT_INDEX, MISS_SHADER_OFFSET, wpos, ray_direction, t_max, depth);

		shadow_factor += lerp(1.0, 0.0, shadow);
	}
#endif //TEST_A

	shadow_factor /= float(MAX_SHADOW_SAMPLES);

#else //SOFT_SHADOWS

	bool shadow = TraceShadowRay(RAY_CONTR_TO_HIT_INDEX, MISS_SHADER_OFFSET, wpos, light_dir, t_max, depth);
	shadow_factor = !shadow;

#endif //SOFT_SHADOWS

	return shadow_factor;
}

#endif //__RT_SHADOWS_FUNCTIONS__

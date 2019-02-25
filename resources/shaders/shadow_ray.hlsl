#include "rt_global.hlsl"

struct ShadowHitInfo
{
  float is_hit;
  float thisvariablesomehowmakeshybridrenderingwork_killme;
};

struct Attributes { };

bool TraceShadowRay(uint idx, float3 origin, float3 direction, float far, unsigned int depth)
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
		~0, // InstanceInclusionMask
		1, // RayContributionToHitGroupIndex
		0, // MultiplierForGeometryContributionToHitGroupIndex
		1, // miss shader index is set to idx but can probably be anything.
		ray,
		payload);

	return payload.is_hit;
}

[shader("closesthit")]
void ShadowClosestHitEntry(inout ShadowHitInfo hit, Attributes bary)
{
    hit.is_hit = true;
}

[shader("miss")]
void ShadowMissEntry(inout ShadowHitInfo hit : SV_RayPayload)
{
    hit.is_hit = false;
}

// Returns amount of shadow, where 1.0 is lit and 0.0 is fully shadowed
float DoShadowRay(float3 wpos, float3 dir, float t_max, inout uint rand_seed, uint depth)
{
	float shadow_factor = 0.0;

#ifdef SOFT_SHADOWS
	[unroll(MAX_SHADOW_SAMPLES)]
	for (uint i = 0; i < MAX_SHADOW_SAMPLES; ++i)
	{
		// Perhaps change randomness to not be purely random, but algorithm-random?
		float3 offset = normalize(float3(nextRand(rand_seed), nextRand(rand_seed), nextRand(rand_seed))) - 0.5;
		// Hard-coded 0.05 is to minimalize the offset a ray gets
		// Should be determined by the area that the light is emitting from
		offset *= 0.05;
		float3 shadow_direction = normalize(dir + offset);

		bool shadow = TraceShadowRay(1, wpos, shadow_direction, t_max, 0);

		shadow_factor += !shadow;
	}

	shadow_factor /= float(MAX_SHADOW_SAMPLES);

#else /* ifdef SOFT_SHADOWS */

	bool shadow = TraceShadowRay(1, wpos, dir, t_max, 0);
	shadow_factor = !shadow;

#endif

	return shadow_factor;
}
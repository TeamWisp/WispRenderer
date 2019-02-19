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

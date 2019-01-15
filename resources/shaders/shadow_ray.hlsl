#include "rt_global.hlsl"

struct ShadowHitInfo
{
  bool is_hit;
};

struct Attributes
{
	float2 uv;
};

bool TraceShadowRay(float3 origin, float3 direction, float far, unsigned int depth, unsigned int seed)
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

	ShadowHitInfo payload = { false };

	// Trace the ray
	TraceRay(
		Scene,
		RAY_FLAG_NONE,
		~0, // InstanceInclusionMask
		1, // RayContributionToHitGroupIndex
		0, // MultiplierForGeometryContributionToHitGroupIndex
		1, // miss shader index
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

#include "rt_global.hlsl"
#include "util.hlsl"

RWTexture2D<float4> output : register(u0); // x: AO value
Texture2D gbuffer_normal : register(t1);
Texture2D gbuffer_depth : register(t2);

struct AOHitInfo
{
  float is_hit;
  float thisvariablesomehowmakeshybridrenderingwork_killme;
};

cbuffer CameraProperties : register(b0)
{
	float4x4 inv_view;
	float4x4 inv_projection;
	float4x4 inv_vp;

	float3 padding;
	float frame_idx;
	float intensity;

	uint shadows_enabled;
	uint reflections_enabled;
	uint ao_enabled;
};

struct Attributes { };

float3 unpack_position(float2 uv, float depth)
{
	// Get world space position
	const float4 ndc = float4(uv * 2.0 - 1.0, depth, 1.0);
	float4 wpos = mul(inv_vp, ndc);
	return (wpos.xyz / wpos.w).xyz;
}

bool TraceAORay(uint idx, float3 origin, float3 direction, float far, unsigned int depth)
{
	// Define a ray, consisting of origin, direction, and the min-max distance values
	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = direction;
	ray.TMin = EPSILON;
	ray.TMax = far;

	AOHitInfo payload = { false, 0 };

	// Trace the ray
	TraceRay(
		Scene,
		RAY_FLAG_NONE,
		~0, // InstanceInclusionMask
		0, // RayContributionToHitGroupIndex
		0, // MultiplierForGeometryContributionToHitGroupIndex
		0, // miss shader index is set to idx but can probably be anything.
		ray,
		payload);

	return payload.is_hit;
}


[shader("raygeneration")]
void AORaygenEntry()
{
    uint rand_seed = initRand(DispatchRaysIndex().x + DispatchRaysIndex().y * DispatchRaysDimensions().x, frame_idx);

	// Texture UV coordinates [0, 1]
	float2 uv = float2(DispatchRaysIndex().xy) / float2(DispatchRaysDimensions().xy - 1);

	// Screen coordinates [0, resolution] (inverted y)
	int2 screen_co = DispatchRaysIndex().xy;

    float3 normal = gbuffer_normal[screen_co].xyz;

	float depth = gbuffer_depth[screen_co].x;
	float3 wpos = unpack_position(float2(uv.x, 1.f - uv.y), depth);

    uint spp = 32;
    float ao_value = 1.0f;
    for(uint i = 0; i< spp; i++)
    {
        ao_value -= (1.0f/float(spp)) * TraceAORay(0, wpos, getCosHemisphereSample(rand_seed, normal), 25.f, 0);
    }

    output[DispatchRaysIndex().xy].x = ao_value;
}

[shader("closesthit")]
void ClosestHitEntry(inout AOHitInfo hit, Attributes bary)
{
    hit.is_hit = true;
}

[shader("miss")]
void MissEntry(inout AOHitInfo hit : SV_RayPayload)
{
    hit.is_hit = false;
}

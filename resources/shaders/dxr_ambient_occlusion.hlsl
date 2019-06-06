#ifndef __DXR_AMBIENT_OCCLUSION_HLSL__
#define __DXR_AMBIENT_OCCLUSION_HLSL__

#include "rand_util.hlsl"
#include "dxr_global.hlsl"

RWTexture2D<float4> output : register(u0); // x: AO value
Texture2D gbuffer_normal : register(t1);
Texture2D gbuffer_depth : register(t2);

struct AOHitInfo
{
  float is_hit;
  float thisvariablesomehowmakeshybridrenderingwork_killme;
};

cbuffer CBData : register(b0)
{
	float4x4 inv_vp;
	float4x4 inv_view;

	float bias;
	float radius;
	float power;
	float max_distance;
	
	float2 padding;
	float frame_idx;
	unsigned int sample_count;
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
	ray.TMin = 0.f;
	ray.TMax = far;

	AOHitInfo payload = { 1.0f, 0.0f };

	// Trace the ray
	TraceRay(
		Scene,
		RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
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
	// Texture UV coordinates [0, 1]
	float2 uv = float2(DispatchRaysIndex().xy) / float2(DispatchRaysDimensions().xy - 1);

    uint rand_seed = initRand(DispatchRaysIndex().x + DispatchRaysIndex().y * DispatchRaysDimensions().x, frame_idx);

	// Screen coordinates [0, resolution] (inverted y)
	int2 screen_co = DispatchRaysIndex().xy;

    float3 normal = gbuffer_normal[screen_co].xyz;
	float depth = gbuffer_depth[screen_co].x;
	float3 wpos = unpack_position(float2(uv.x, 1.f - uv.y), depth);

	float3 camera_pos = float3(inv_view[0][3], inv_view[1][3], inv_view[2][3]);
	float cam_distance = length(wpos-camera_pos);
	if(cam_distance < max_distance)
	{
		//SPP decreases the closer a pixel is to the max distance
		//Total is always calculated using the full sample count to have further pixels less occluded
		int spp = min(sample_count, round(sample_count * ((max_distance - cam_distance)/max_distance))); 
		int ao_value = sample_count;
		for(uint i = 0; i< spp; i++)
		{
			 ao_value -= TraceAORay(0, wpos + normal * bias , getCosHemisphereSample(rand_seed, normal), radius, 0);
		}
		output[DispatchRaysIndex().xy].x = pow(ao_value/float(sample_count), power);
	}
	else
	{
		output[DispatchRaysIndex().xy].x = 1.f;
	}

}

[shader("miss")]
void MissEntry(inout AOHitInfo hit : SV_RayPayload)
{
    hit.is_hit = 0.0f;
}

#endif //__DXR_AMBIENT_OCCLUSION_HLSL__
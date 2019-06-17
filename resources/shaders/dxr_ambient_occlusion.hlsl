#ifndef __DXR_AMBIENT_OCCLUSION_HLSL__
#define __DXR_AMBIENT_OCCLUSION_HLSL__

#include "rand_util.hlsl"
#include "dxr_global.hlsl"
#include "dxr_functions.hlsl"
#include "dxr_structs.hlsl"
#include "material_util.hlsl"

RWTexture2D<float4> output : register(u0); // x: AO value

ByteAddressBuffer g_indices : register(t1);
StructuredBuffer<Vertex> g_vertices : register(t2);
StructuredBuffer<Material> g_materials : register(t3);
StructuredBuffer<Offset> g_offsets : register(t4);

Texture2D g_textures[1000] : register(t5);
Texture2D gbuffer_normal : register(t1005);
Texture2D gbuffer_depth : register(t1006);

SamplerState s0 : register(s0);

typedef BuiltInTriangleIntersectionAttributes Attributes;

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
	// Texture UV coordinates [0, 1]
	float2 uv = float2(DispatchRaysIndex().xy) / float2(DispatchRaysDimensions().xy - 1);

    uint rand_seed = initRand(DispatchRaysIndex().x + DispatchRaysIndex().y * DispatchRaysDimensions().x, frame_idx);

	// Screen coordinates [0, resolution] (inverted y)
	int2 screen_co = DispatchRaysIndex().xy;

    float3 normal = gbuffer_normal[screen_co].xyz;
	float depth = gbuffer_depth[screen_co].x;
	float3 wpos = unpack_position(float2(uv.x, 1.f - uv.y), depth, inv_vp);

	float3 camera_pos = float3(inv_view[0][3], inv_view[1][3], inv_view[2][3]);
	float cam_distance = length(wpos-camera_pos);
	if(cam_distance < max_distance)
	{
		//SPP decreases the closer a pixel is to the max distance
		//Total is always calculated using the full sample count to have further pixels less occluded
		int spp = min(sample_count, round(sample_count * ((max_distance - cam_distance) / max_distance)));
		int ao_value = sample_count;
		for(uint i = 0; i < spp; i++)
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

[shader("closesthit")]
void AOClosestHitEntry(inout AOHitInfo hit, in Attributes attr)
{
	hit.is_hit = 1.0f;
}

[shader("miss")]
void AOMissEntry(inout AOHitInfo hit : SV_RayPayload)
{
    hit.is_hit = 0.0f;
}

[shader("anyhit")]
void AOAnyHitEntry(inout AOHitInfo hit, in Attributes attr)
{
//#define FALLBACK
#ifndef FALLBACK
	// Calculate the essentials
	const Offset offset = g_offsets[InstanceID()];
	const Material material = g_materials[offset.material_idx];
	const float index_offset = offset.idx_offset;
	const float vertex_offset = offset.vertex_offset;

	// Find first index location
	const uint index_size = 4;
	const uint indices_per_triangle = 3;
	const uint triangle_idx_stride = indices_per_triangle * index_size;

	uint base_idx = PrimitiveIndex() * triangle_idx_stride;
	base_idx += index_offset * 4; // offset the start

	uint3 indices = Load3x32BitIndices(g_indices, base_idx);
	indices += float3(vertex_offset, vertex_offset, vertex_offset); // offset the start

	// Gather triangle vertices
	const Vertex v0 = g_vertices[indices.x];
	const Vertex v1 = g_vertices[indices.y];
	const Vertex v2 = g_vertices[indices.z];

	//Get data from VBO
	float2 uv = HitAttribute(float3(v0.uv, 0), float3(v1.uv, 0), float3(v2.uv, 0), attr).xy;
	uv.y = 1.0f - uv.y;

	OutputMaterialData output_data = InterpretMaterialDataRT(material.data,
		g_textures[material.albedo_id],
		g_textures[material.normal_id],
		g_textures[material.roughness_id],
		g_textures[material.metalicness_id],
		g_textures[material.emissive_id],
		g_textures[material.ao_id],
		0,
		s0,
		uv);

	float alpha = output_data.alpha;

	if (alpha < 0.5f)
	{
		IgnoreHit();
	}
	else
	{
		AcceptHitAndEndSearch();
	}
#else
	AcceptHitAndEndSearch();
#endif
}

#endif //__DXR_AMBIENT_OCCLUSION_HLSL__
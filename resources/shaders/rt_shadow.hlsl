#include "pbr_util.hlsl"

#define MAX_RECURSION 1

struct Light
{
	float3 pos;			//Position in world space for spot & point
	float radius;			//Radius for point, height for spot

	float3 color;			//Color
	uint tid;			//Type id; light_type_x

	float3 dir;			//Direction for spot & directional
	float angel;			//Angle for spot; in radians
};

struct Vertex
{
	float3 pos;
	float2 uv;
	float3 normal;
	float3 tangent;
	float3 bitangent;
};

struct Material
{
	float4x4 model;
	float idx_offset;
	float vertex_offset;
	float albedo_id;
	float normal_id;
};

RWTexture2D<float4> gOutput : register(u0);
RaytracingAccelerationStructure Scene : register(t0, space0);
ByteAddressBuffer g_indices : register(t1);
StructuredBuffer<Light> lights : register(t2);
StructuredBuffer<Vertex> g_vertices : register(t3);
StructuredBuffer<Material> g_materials : register(t4);

Texture2D g_textures[20] : register(t5);
Texture2D gbuffer_albedo : register(t31);
Texture2D gbuffer_normal : register(t32);
Texture2D gbuffer_depth : register(t33);
SamplerState s0 : register(s0);

typedef BuiltInTriangleIntersectionAttributes MyAttributes;
struct HitInfo
{
	float shadow_factor;
	unsigned int depth;
};

cbuffer CameraProperties : register(b0)
{
	float4x4 inv_view;
	float4x4 inv_projection;
};

struct Ray
{
	float3 origin;
	float3 direction;
};

static const uint light_type_point = 0;
static const uint light_type_directional = 1;
static const uint light_type_spot = 2;

uint3 Load3x32BitIndices(uint offsetBytes)
{
	// Load first 2 indices
	return g_indices.Load3(offsetBytes);
}

// Retrieve hit world position.
float3 HitWorldPosition()
{
	return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

float TraceShadowRay(inout HitInfo payload, float3 origin, float3 direction, unsigned int depth)
{
	if (depth >= MAX_RECURSION)
	{
		return float(1.0);
	}

	// Define a ray, consisting of origin, direction, and the min-max distance values
	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = direction;
	ray.TMin = 0.00001;
	ray.TMax = 10000.0;

	// Trace the ray
	TraceRay(
		Scene,
		RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH
		| RAY_FLAG_FORCE_OPAQUE,
		~0, // InstanceInclusionMask
		0, // RayContributionToHitGroupIndex
		1, // MultiplierForGeometryContributionToHitGroupIndex
		0, // miss shader index
		ray,
		payload);

	return payload.shadow_factor;
}

float3 unpack_position(float2 uv, float depth, float4x4 proj_inv, float4x4 view_inv)
{
	// Get view space position
	const float4 ndc = float4(uv * 2.0 -1.0, depth, 1.0);
	float4 vpos = mul(proj_inv, ndc);
	vpos /= vpos.w;
	// Get and return projection space position
	float4 wpos = mul(view_inv, float4(vpos.xyz, 1.0));
	return wpos.xyz;
}

[shader("raygeneration")]
void RaygenEntry()
{
	// HitInfo payload = {1.0, depth};


	// Screen coordinates [0, 1]
	float2 screenCo = float2(DispatchRaysIndex().xy) / float2(DispatchRaysDimensions().xy);

	// Texture UV coordinates [0, 1] (inverted y)
	int2 uv = DispatchRaysIndex().xy;
	uv.y = DispatchRaysDimensions().y - uv.y;

	// Get g-buffer information
	float3 albedo = gbuffer_albedo[DispatchRaysIndex().xy].xyz;
	float3 norm = gbuffer_normal[DispatchRaysIndex().xy].xyz;
	float depth = gbuffer_depth[uv.xy].x;

	// Get world position
	float3 wpos = unpack_position(screenCo, depth, inv_projection, inv_view);


	// Output world position
	gOutput[DispatchRaysIndex().xy] = float4(wpos.xyz, 1.0);
}

float3 HitAttribute(float3 a, float3 b, float3 c, BuiltInTriangleIntersectionAttributes attr)
{
	float3 vertexAttribute[3];
	vertexAttribute[0] = a;
	vertexAttribute[1] = b;
	vertexAttribute[2] = c;

	return vertexAttribute[0] +
		attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
		attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}

[shader("closesthit")]
void ClosestHitEntry(inout HitInfo payload, in MyAttributes attr)
{
	payload.shadow_factor *= 0.3;
}


[shader("miss")]
void MissEntry(inout HitInfo payload)
{
	// NOT USED
}
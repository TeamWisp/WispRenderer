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
Texture2D gbuffer_albedo : register(t30);
Texture2D gbuffer_normal : register(t31);
Texture2D gbuffer_depth : register(t32);
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
	float4x4 inv_vp;
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

float TraceShadowRay(float3 origin, float3 direction, unsigned int depth)
{
	if (depth >= MAX_RECURSION)
	{
		return float(1.0);
	}

	HitInfo payload = {1.0, depth};

	// Define a ray, consisting of origin, direction, and the min-max distance values
	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = direction;
	ray.TMin = 0.0000;
	ray.TMax = 10000.0;

	// Trace the ray
	TraceRay(
		Scene,
RAY_FLAG_NONE,
		//RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH
		// RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
		~0, // InstanceInclusionMask
		0, // RayContributionToHitGroupIndex
		1, // MultiplierForGeometryContributionToHitGroupIndex
		0, // miss shader index
		ray,
		payload);

	return payload.shadow_factor;
}

float3 unpack_position(float2 uv, float depth)
{
	// Get world space position
	const float4 ndc = float4(uv * 2.0 - 1.0, depth, 1.0);
	float4 wpos = mul(inv_vp, ndc);
	return (wpos.xyz / wpos.w).xyz;
}

float3 unpack_direction(float3 dir)
{
	// Get world space normal
	const float4 vnormal = float4(dir.xyz, 0.0);
	float4 wnormal = mul(inv_view, vnormal);
	return wnormal.xyz;
}

[shader("raygeneration")]
void RaygenEntry()
{
	// Texture UV coordinates [0, 1>
	float2 uv = float2(DispatchRaysIndex().xy) / float2(DispatchRaysDimensions().xy - 1);
	

	// Screen coordinates [0, resolution] (inverted y)
	int2 screenCo = DispatchRaysIndex().xy;
	screenCo.y = (DispatchRaysDimensions().y - screenCo.y - 1);

	// Get g-buffer information
	float3 albedo = gbuffer_albedo[screenCo.xy].xyz;
	float3 normal = unpack_direction(gbuffer_normal[screenCo.xy].xyz);
	float depth = gbuffer_depth[screenCo.xy].x;

	// Get world position
	const float n = 0.1f;
	const float f = 25.0f;
	const float z = (2 * n) / (f + n - depth * (f - n)) / f;

	float3 wpos = unpack_position(uv, depth) + (normal * 5e-3f);

	// Set temp light direction
	float3 light_dir = normalize(float3(0.0, -1.0, 0.0));
	//float3 light_dir = normalize(float3(lights[0].dir.xyz));

	// Trace shadow ray
	float shadow_factor = TraceShadowRay(wpos.xyz, light_dir, 0);

	// Output world position
	gOutput[DispatchRaysIndex().xy] = float4(albedo.xyz * shadow_factor, 1.0);
	//gOutput[DispatchRaysIndex().xy] = float4(wpos.xyz, 1.0);
	//gOutput[DispatchRaysIndex().xy] = float4(normal.xyz, 1.0);
	//gOutput[DispatchRaysIndex().xy] = float4((wpos.xyz+ 1.0) * 0.5, 1.0);
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
	payload.shadow_factor = 1.0;
}
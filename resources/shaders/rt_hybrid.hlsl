#include "pbr_util.hlsl"

#define MAX_RECURSION 1

struct Light
{
	float3 pos;			//Position in world space for spot & point
	float radius;		//Radius for point, height for spot

	float3 color;		//Color
	uint tid;			//Type id; light_type_x

	float3 dir;			//Direction for spot & directional
	float angle;		//Angle for spot; in radians
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
struct ShadowHitInfo
{
	float2 padding;
	float shadow_factor;
};

struct ReflectionHitInfo
{
	float3 color;
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

float TraceShadowRay(float3 origin, float3 direction)
{
	ShadowHitInfo payload = { float2(0, 0), 1 };

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

float3 TraceReflectionRay(float3 origin, float3 direction)
{
	ReflectionHitInfo payload = { float3(0, 0, 1) };

	float epsilon = 0.05;

	// Define a ray, consisting of origin, direction, and the min-max distance values
	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = direction;
	ray.TMin = epsilon;
	ray.TMax = 10000.0;

	// Trace the ray
	TraceRay(
		Scene,
		RAY_FLAG_NONE,
		//RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH
		// RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
		~0, // InstanceInclusionMask
		1, // RayContributionToHitGroupIndex
		1, // MultiplierForGeometryContributionToHitGroupIndex
		1, // miss shader index
		ray,
		payload);

	return payload.color;
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

float DoShadow(float3 wpos, float depth, float3 normal)
{

	//TODO: Calculate epsilon depending on distance

	const float n = 0.1f;
	const float f = 25.0f;
	const float z = (2 * n) / (f + n - depth * (f - n)) / f;

	float epsilon = 0.05;

	//Calculate origin

	float3 origin = wpos + normal * epsilon;

	//Calculate shadow factor

	float shadow_factor = 1.0;
	uint light_count = lights[0].tid >> 2;	//Light count is stored in 30 upper-bits of first light

	for (uint i = 0; i < light_count; i++)
	{
		float3 light_dir = normalize(lights[i].dir);

		// Trace shadow ray
		shadow_factor *= TraceShadowRay(origin, light_dir);
	}

	return shadow_factor;
}

float3 DoReflection(float3 wpos, float3 normal, float roughness, float metallic)
{

	// Get direction to camera

	float3 cpos = float3(inv_view[0][3], inv_view[1][3], inv_view[2][3]);

	float3 cdir = wpos - cpos;
	float cdist = length(cdir);
	cdir /= cdist;

	// Calculate ray info

	float3 reflected = reflect(cdir, normal);

	// Shoot reflection ray

	float3 reflection = TraceReflectionRay(wpos, reflected);

	// TODO: Get reflectiveness from roughness and metallic

	float reflectiveness = 1;

	return reflection * reflectiveness;
}

[shader("raygeneration")]
void RaygenEntry()
{
	// Texture UV coordinates [0, 1>
	float2 uv = float2(DispatchRaysIndex().xy) / float2(DispatchRaysDimensions().xy - 1);

	// Screen coordinates [0, resolution] (inverted y)
	int2 screen_co = DispatchRaysIndex().xy;
	screen_co.y = (DispatchRaysDimensions().y - screen_co.y - 1);

	// Get g-buffer information
	float4 albedo_roughness = gbuffer_albedo[screen_co];
	float4 normal_metallic = gbuffer_normal[screen_co];

	// Unpack G-Buffer
	float depth = gbuffer_depth[screen_co].x;
	float3 wpos = unpack_position(uv, depth);
	float3 albedo = albedo_roughness.rgb;
	float roughness = albedo_roughness.w;
	float3 normal = unpack_direction(normal_metallic.xyz);
	float metallic = normal_metallic.w;

	// Do lighting

	float shadow_factor = DoShadow(wpos, depth, normal);
	float3 reflection = DoReflection(wpos, normal, metallic, roughness);

	gOutput[DispatchRaysIndex().xy] = float4((reflection) * albedo, 1);

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

//Shadows

[shader("closesthit")]
void ClosestHitEntry(inout ShadowHitInfo payload, in MyAttributes attr)
{
	payload.shadow_factor = 0.75;
}

[shader("miss")]
void MissEntry(inout ShadowHitInfo payload)
{
	payload.shadow_factor = 1.0;
}

//Reflections

[shader("closesthit")]
void ReflectionHit(inout ReflectionHitInfo payload, in MyAttributes attr)
{
	const Material material = g_materials[InstanceID()];
	const float index_offset = material.idx_offset;
	const float vertex_offset = material.vertex_offset;

	// Find first index location
	const uint index_size = 4;
	const uint indices_per_triangle = 3;
	const uint triangle_idx_stride = indices_per_triangle * index_size;

	uint base_idx = PrimitiveIndex() * triangle_idx_stride;
	base_idx += index_offset * 4; // offset the start

	uint3 indices = Load3x32BitIndices(base_idx);
	indices += float3(vertex_offset, vertex_offset, vertex_offset); // offset the start

	// Gather triangle vertices
	const Vertex v0 = g_vertices[indices.x];
	const Vertex v1 = g_vertices[indices.y];
	const Vertex v2 = g_vertices[indices.z];
	
	//Get uv
	float3 uv = HitAttribute(float3(v0.uv, 0), float3(v1.uv, 0), float3(v2.uv, 0), attr);
	float3 albedo = g_textures[material.albedo_id].SampleLevel(s0, uv, 0).xyz;

	payload.color = albedo;
}

[shader("miss")]
void ReflectionMiss(inout ReflectionHitInfo payload)
{
	payload.color = float3(1, 0, 0);
}
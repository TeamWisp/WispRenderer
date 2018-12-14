#include "pbr_util.hlsl"

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
StructuredBuffer<Vertex> g_vertices : register(t2);
StructuredBuffer<Material> g_materials : register(t3);

Texture2D g_textures[20] : register(t4);
SamplerState s0 : register(s0);

typedef BuiltInTriangleIntersectionAttributes MyAttributes;
struct HitInfo
{
	float4 color;
};

cbuffer CameraProperties : register(b0)
{
	float4x4 view;
	float4x4 inv_projection_view;
	float3 camera_position;
	float padding;

	float light_radius;
	float metal;
	float roughness;
	float intensity;
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

inline Ray GenerateCameraRay(uint2 index, in float3 cameraPosition, in float4x4 projectionToWorld)
{
    float2 xy = index + 0.5f; // center in the middle of the pixel.
    float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

    // Unproject the pixel coordinate into a world positon.
    float4 world = mul(float4(screenPos, 0, 1), projectionToWorld);
    world.xyz /= world.w;

    Ray ray;
    ray.origin = cameraPosition;
    ray.direction = normalize(world.xyz - ray.origin);

    return ray;
}

// Retrieve hit world position.
float3 HitWorldPosition()
{
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

[shader("raygeneration")]
void RaygenEntry()
{
	// Initialize the ray payload
	HitInfo payload;
	payload.color = float4(1, 1, 1, 1);

	Ray temp_ray = GenerateCameraRay(DispatchRaysIndex().xy, camera_position, inv_projection_view);

	// Define a ray, consisting of origin, direction, and the min-max distance values
	RayDesc ray;
	ray.Origin = temp_ray.origin;
	ray.Direction = temp_ray.direction;
	ray.TMin = 0;
	ray.TMax = 10000.0;

	// Trace the ray
	TraceRay(
		Scene,
		RAY_FLAG_NONE,
		~0, // InstanceInclusionMask
		0, // RayContributionToHitGroupIndex
		1, // MultiplierForGeometryContributionToHitGroupIndex
		0, // miss shader index
		ray,
		payload);

	gOutput[DispatchRaysIndex().xy] = payload.color;
}

[shader("miss")]
void MissEntry(inout HitInfo payload)
{
	payload.color = float4(170.0f / 255.f, 203.0f / 255.f, 1.0f, 1.f);
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

float calc_attenuation(float r, float d) {
	return 1.0f - smoothstep(r * 0, r, d);
}

float3 ShadeLight(float3 vpos, float3 V, float3 albedo, float3 normal)
{
	uint tid = /*light.tid & 3*/ 0;

	float3 light_pos = float3(0, 0, 0);
	float3 light_dir = float3(0, 0, 1);
	float3 light_rad = light_radius;
	float3 light_col = float3(1, 1, 1);

	//Light direction (constant with directional, position dependent with other)
	//float3 L = (lerp(light_pos - vpos, light_dir, tid == light_type_directional));
	//float light_dist = length(L);
	//L /= light_dist;

	float3 dir = light_pos - vpos;
	const float distance = length(dir);
	dir = normalize(dir);

	float range = light_rad;
	const float attenuation = calc_attenuation(range, distance);
	const float3 radiance = (intensity * light_col) * attenuation; 

	//Attenuation & spot intensity (only used with point or spot)
	//float attenuation = lerp(1.0f - smoothstep(0, light_rad, light_dist), 1, tid == light_type_directional);
	//float3 radiance = (light_col * intensity) * attenuation;

	float3 lighting = BRDF(dir, V, normal, metal, roughness, albedo, radiance, light_col);

	return lighting;
}

[shader("closesthit")]
void ClosestHitEntry(inout HitInfo payload, in MyAttributes attr)
{
	// Calculate the essentials
	const Material material = g_materials[InstanceID()];
	const float3 hit_pos = HitWorldPosition();
	const float index_offset = material.idx_offset;
	const float vertex_offset = material.vertex_offset;
	const float4x4 model_matrix = material.model;
	
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

	// Calculate actual "fragment" attributes.
	float3 frag_pos = HitAttribute(v0.pos, v1.pos, v2.pos, attr);
	float3 normal = normalize(HitAttribute(v0.normal, v1.normal, v2.normal, attr));
	float3 uv = HitAttribute(float3(v0.uv, 0), float3(v1.uv, 0), float3(v2.uv, 0), attr);

	float3 world_normal = normalize(mul(model_matrix, float4(normal, 1))).xyz;

	const float3 albedo = g_textures[material.albedo_id].SampleLevel(s0, uv, 0).xyz;
	const float3 normal_t = g_textures[material.normal_id].SampleLevel(s0, uv, 0).xyz;

	// Variables
	//float4x4 vm = mul(view, float4(hit_pos, 1));
	float3 V = normalize(camera_position - hit_pos);

	// Diffuse
    float3 lighting = ShadeLight(hit_pos, V, albedo, world_normal);

	// Output
	payload.color = float4(lighting, 1);
}

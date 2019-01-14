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
	float roughness_id;
	float metalicness_id;
	float2 padding;
};

RWTexture2D<float4> gOutput : register(u0);
RaytracingAccelerationStructure Scene : register(t0, space0);
ByteAddressBuffer g_indices : register(t1);
StructuredBuffer<Light> lights : register(t2);
StructuredBuffer<Vertex> g_vertices : register(t3);
StructuredBuffer<Material> g_materials : register(t4);

Texture2D g_textures[20] : register(t5);
Texture2D gbuffer_albedo : register(t25);
Texture2D gbuffer_normal : register(t26);
Texture2D gbuffer_depth : register(t27);
SamplerState s0 : register(s0);

typedef BuiltInTriangleIntersectionAttributes MyAttributes;
struct ShadowHitInfo
{
	bool shadow_hit;
};

struct ReflectionHitInfo
{
	float3 origin;
	float3 color;
};

cbuffer CameraProperties : register(b0)
{
	float4x4 inv_view;
	float4x4 inv_projection;
	float4x4 inv_vp;

	float3 padding;
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

//TODO: Replace sky_color by sampling an envmap
static const float3 sky_color = float3(0x1E / 255.f, 0x90 / 255.f, 0xFF / 255.f);

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

bool TraceShadowRay(float3 origin, float3 direction)
{
	ShadowHitInfo payload = { false };

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

	return payload.shadow_hit;
}

float3 TraceReflectionRay(float3 origin, float3 direction)
{
	float epsilon = 0.05;

	ReflectionHitInfo payload = { origin + direction * epsilon, float3(0, 0, 1) };

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

float calc_attenuation(float r, float d)
{
	return 1.0f - smoothstep(r * 0, r, d);
}

float3 ShadeLight(float3 vpos, float3 V, float3 albedo, float3 normal, float roughness, float metal, Light light)
{
	uint tid = light.tid & 3;

	//Light direction (constant with directional, position dependent with other)
	float3 L = (lerp(light.pos - vpos, light.dir, tid == light_type_directional));
	float light_dist = length(L);
	L /= light_dist;

	float range = light.radius;
	const float attenuation = calc_attenuation(range, light_dist);
	const float3 radiance = (intensity * light.color) * attenuation;

	//Attenuation & spot intensity (only used with point or spot)
	//float attenuation = lerp(1.0f - smoothstep(0, light_rad, light_dist), 1, tid == light_type_directional);
	//float3 radiance = (light_col * intensity) * attenuation;

	float3 lighting = BRDF(L, V, normal, metal, roughness, albedo, radiance, light.color);

	float3 origin = vpos + normal * 0.05;
	bool  is_shadow = TraceShadowRay(origin, L);
	lighting = lerp(lighting, float3(0, 0, 0), is_shadow);

	return lighting;
}

float3 ShadePixel(float3 vpos, float3 V, float3 albedo, float3 normal, float roughness, float metal)
{
	uint light_count = lights[0].tid >> 2;	//Light count is stored in 30 upper-bits of first light

	float ambient = 0.1f;
	float3 res = float3(ambient, ambient, ambient);

	for (uint i = 0; i < light_count; i++)
	{
		res += ShadeLight(vpos, V, albedo, normal, roughness, metal, lights[i]);
	}

	return res * albedo;
}

float DoShadow(float3 wpos, float depth, float3 normal)
{
	float epsilon = 0.05;

	//Calculate origin

	float3 origin = wpos + normal * epsilon;

	//Calculate shadow factor

	float shadow_factor = 1.0;
	uint light_count = lights[0].tid >> 2;	//Light count is stored in 30 upper-bits of first light

	for (uint i = 0; i < light_count; i++)
	{
		uint tid = lights[i].tid & 3;
		float3 L = (lerp(lights[i].pos - wpos, lights[i].dir, tid == light_type_directional));
		float light_dist = length(L);
		L /= light_dist;

		// Trace shadow ray
		if (TraceShadowRay(origin, L))
		{
			shadow_factor *= 0.75;
		}
	}

	return shadow_factor;
}

float3 DoReflection(float3 wpos, float3 normal, float roughness, float metallic, float3 albedo)
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

	// TODO: Roughness

	// TODO: Calculate reflection combined with fresnel

	float cosTheta = max(-dot(normal, cdir), 0);
	float fresnel = pow(1 - cosTheta, 5 / (4.9 * metallic + 0.1));

	float3 fresnel_reflection = lerp(albedo, reflection, fresnel);

	return fresnel_reflection;
}

[shader("raygeneration")]
void RaygenEntry()
{
	// Texture UV coordinates [0, 1]
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

	if (length(normal) == 0)		//TODO: Could be optimized by only marking pixels that need lighting, but that would require execute rays indirect
	{
		gOutput[DispatchRaysIndex().xy] = float4(sky_color, 1);
		return;
	}

	// Do lighting

	float3 cpos = float3(inv_view[0][3], inv_view[1][3], inv_view[2][3]);
	float3 V = normalize(cpos - wpos);

	float3 lighting = ShadePixel(wpos, V, albedo, normal, roughness, metallic);
	float3 reflection = DoReflection(wpos, normal, metallic, roughness, lighting);

	gOutput[DispatchRaysIndex().xy] = float4(reflection, 1);

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
	payload.shadow_hit = true;
}

[shader("miss")]
void MissEntry(inout ShadowHitInfo payload)
{
	payload.shadow_hit = false;
}

//Reflections

[shader("closesthit")]
void ReflectionHit(inout ReflectionHitInfo payload, in MyAttributes attr)
{
	const Material material = g_materials[InstanceID()];
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
	
	//Get data from VBO
	float3 uvw = HitAttribute(float3(v0.uv, 0), float3(v1.uv, 0), float3(v2.uv, 0), attr);
	float2 uv = uvw.xy;
	float3 albedo = g_textures[material.albedo_id].SampleLevel(s0, uv, 0).xyz;
	float roughness = g_textures[material.roughness_id].SampleLevel(s0, uv, 0).x;
	float metal = g_textures[material.metalicness_id].SampleLevel(s0, uv, 0).x;

	//Direction & position

	const float3 hit_pos = HitWorldPosition();
	float3 V = normalize(payload.origin - hit_pos);

	//Normal mapping
	float3 normal = normalize(HitAttribute(v0.normal, v1.normal, v2.normal, attr));
	float3 tangent = HitAttribute(v0.tangent, v1.tangent, v2.tangent, attr);
	float3 bitangent = HitAttribute(v0.bitangent, v1.bitangent, v2.bitangent, attr);

	float3 N = normalize(mul(model_matrix, float4(normal, 0)).xyz);
	float3 T = normalize(mul(model_matrix, float4(tangent, 0)).xyz);
	float3 B = normalize(mul(model_matrix, float4(bitangent, 0)).xyz);
	float3x3 TBN = float3x3(T, B, N);

	float3 normal_t = (g_textures[material.normal_id].SampleLevel(s0, uv, 0).xyz) * 2.0 - float3(1.0, 1.0, 1.0);

	float3 fN = normalize(mul(normal_t, TBN));
	if (dot(fN, V) <= 0.0f) fN = -fN;

	//Shading
	payload.color = ShadePixel(hit_pos, V, albedo, normal, roughness, metal);
}

[shader("miss")]
void ReflectionMiss(inout ReflectionHitInfo payload)
{
	payload.color = sky_color;
}
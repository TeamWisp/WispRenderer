#define LIGHTS_REGISTER register(t2)
#include "pbr_util.hlsl"
#include "lighting.hlsl"

struct Vertex
{
	float3 pos;
	float2 uv;
	float3 normal;
	float3 tangent;
	float3 bitangent;
	float3 color;
};

struct Material
{
	float albedo_id;
	float normal_id;
	float roughness_id;
	float metalicness_id;
};

struct Offset
{
    float material_idx;
    float idx_offset;
    float vertex_offset;
};

RWTexture2D<float4> gOutput : register(u0);
ByteAddressBuffer g_indices : register(t1);
StructuredBuffer<Vertex> g_vertices : register(t3);
StructuredBuffer<Material> g_materials : register(t4);
StructuredBuffer<Offset> g_offsets : register(t5);

Texture2D g_textures[20] : register(t8);
Texture2D gbuffer_albedo : register(t28);
Texture2D gbuffer_normal : register(t29);
Texture2D gbuffer_depth : register(t30);
Texture2D skybox : register(t6);
TextureCube irradiance_map : register(t7);
SamplerState s0 : register(s0);

typedef BuiltInTriangleIntersectionAttributes MyAttributes;

struct ReflectionHitInfo
{
	float3 origin;
	float3 color;
	unsigned int seed;
};

cbuffer CameraProperties : register(b0)
{
	float4x4 inv_view;
	float4x4 inv_projection;
	float4x4 inv_vp;

	float2 padding;
	float frame_idx;
	float intensity;
};

struct Ray
{
	float3 origin;
	float3 direction;
};

uint3 Load3x32BitIndices(uint offsetBytes)
{
	// Load first 2 indices
	return g_indices.Load3(offsetBytes);
}

// Initialize random
uint initRand(uint val0, uint val1, uint backoff = 16)
{
	uint v0 = val0, v1 = val1, s0 = 0;

	[unroll]
	for (uint n = 0; n < backoff; n++)
	{
		s0 += 0x9e3779b9;
		v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
		v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
	}
	return v0;
}

// Get new random float [0, 1]
float nextRand(inout uint s)
{
	s = (1664525u * s + 1013904223u);
	return float(s & 0x00FFFFFF) / float(0x01000000);
}

// Retrieve hit world position.
float3 HitWorldPosition()
{
	return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

<<<<<<< HEAD
bool TraceShadowRay(float3 origin, float3 direction, float t_max)
{
	ShadowHitInfo payload = { false, 1 };

	// Define a ray, consisting of origin, direction, and the min-max distance values
	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = direction;
	ray.TMin = 0.0000;
	ray.TMax = t_max;

	// Trace the ray
	TraceRay(
		Scene,
		// TODO: Change flags if transparency is added
		RAY_FLAG_FORCE_OPAQUE // Treat all geometry as opaque.
		| RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, // Accept first hit
		~0, // InstanceInclusionMask
		0, // RayContributionToHitGroupIndex
		1, // MultiplierForGeometryContributionToHitGroupIndex
		0, // miss shader index
		ray,
		payload);

	return payload.shadow_hit;
}

float3 TraceReflectionRay(float3 origin, float3 norm, float3 direction, uint rand_seed)
=======
float3 TraceReflectionRay(float3 origin, float3 norm, float3 direction)
>>>>>>> master
{
	origin += norm * EPSILON;

	ReflectionHitInfo payload = {origin, float3(0, 0, 1), rand_seed };

	// Define a ray, consisting of origin, direction, and the min-max distance values
	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = direction;
	ray.TMin = 0;
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

	return payload.color;
}

float3 unpack_position(float2 uv, float depth)
{
	// Get world space position
	const float4 ndc = float4(uv * 2.0 - 1.0, depth, 1.0);
	float4 wpos = mul(inv_vp, ndc);
	return (wpos.xyz / wpos.w).xyz;
}

<<<<<<< HEAD
float calc_attenuation(float r, float d)
{
	return 1.0f - smoothstep(r * 0, r, d);
}

float3 ShadeLight(float3 wpos, float3 V, float3 albedo, float3 normal, float roughness, float metal, Light light, inout uint rand_seed)
{
	uint tid = light.tid & 3;

	//Light direction (constant with directional, position dependent with other)
	float3 L = (lerp(light.pos - wpos, light.dir, tid == light_type_directional));
	float light_dist = length(L);
	L /= light_dist;

	//Spot intensity (only used with spot; but always calculated)
	float min_cos = cos(light.angle);
	float max_cos = lerp(min_cos, 1, 0.5f);
	float cos_angle = dot(light.dir, -L);
	float spot_intensity = lerp(smoothstep(min_cos, max_cos, cos_angle), 1, tid != light_type_spot);

	//Attenuation & spot intensity (only used with point or spot)
	float attenuation = lerp(1.0f - smoothstep(0, light.radius, light_dist), 1, tid == light_type_directional);

	const float3 radiance = intensity * spot_intensity * light.color * attenuation;

	float3 lighting = BRDF(L, V, normal, metal, roughness, albedo, radiance, light.color);

	// Check if pixel is shaded
	float3 origin = wpos + normal * epsilon;
	float t_max = lerp(light_dist, 10000.0, tid == light_type_directional);
	//bool is_shadow = TraceShadowRay(origin, L, t_max);

	// Offset shadow ray direction to get soft-shadows
	float shadow_factor = 0.0;
	[unroll]
	for (uint n = 0; n < MAX_SHADOW_SAMPLES; n++)
	{
		float3 offset = float3(nextRand(rand_seed), nextRand(rand_seed), nextRand(rand_seed)) - 0.5;
		offset *= 0.05;
		float3 shadow_direction = normalize(L + offset);

		bool is_shadow = TraceShadowRay(origin, shadow_direction, t_max);

		shadow_factor += lerp(1.0, 0.0, is_shadow);
	}

	shadow_factor /= float(MAX_SHADOW_SAMPLES);

	lighting *= shadow_factor;

	return lighting;
	//return lerp(lighting, float3(0, 0, 0), is_shadow);
}

float3 ShadePixel(float3 pos, float3 V, float3 albedo, float3 normal, float roughness, float metal, inout uint rand_seed)
{
	uint light_count = lights[0].tid >> 2;	//Light count is stored in 30 upper-bits of first light

	float ambient = 0.1f;
	float3 res = float3(ambient, ambient, ambient);

	for (uint i = 0; i < light_count; i++)
	{
		res += ShadeLight(pos, V, albedo, normal, roughness, metal, lights[i], rand_seed);
	}

	return res * albedo;
}


=======
>>>>>>> master
float3 ReflectRay(float3 v1, float3 v2)
{
	return (v2 * ((2.f * dot(v1, v2))) - v1);
}

float3 DoReflection(float3 wpos, float3 V, float3 normal, float roughness, float metallic, float3 albedo, float3 lighting, uint rand_seed)
{

	// Calculate ray info

	float3 reflected = ReflectRay(V, normal);

	// Shoot reflection ray
<<<<<<< HEAD

	float3 reflection = TraceReflectionRay(wpos, normal, reflected, rand_seed);

	// Calculate reflection combined with fresnel

	const float3 F = F_SchlickRoughness(max(dot(normal, V), 0.0), metallic, albedo, roughness);
	const float3 kS = F;
	float3 kD = 1.0 - kS;
	kD *= 1.0 - metallic;

	const float3 specular = F * reflection;

	return specular + lighting;
=======
	float3 reflection = TraceReflectionRay(wpos, normal, reflected);
	return reflection;
>>>>>>> master
}

#define M_PI 3.14159265358979

[shader("raygeneration")]
void RaygenEntry()
{
	uint rand_seed = initRand(DispatchRaysIndex().x + DispatchRaysIndex().y * DispatchRaysDimensions().x, frame_idx);

	// Texture UV coordinates [0, 1]
	float2 uv = float2(DispatchRaysIndex().xy) / float2(DispatchRaysDimensions().xy - 1);

	// Screen coordinates [0, resolution] (inverted y)
	int2 screen_co = DispatchRaysIndex().xy;

	// Get g-buffer information
	float4 albedo_roughness = gbuffer_albedo[screen_co];
	float4 normal_metallic = gbuffer_normal[screen_co];

	// Unpack G-Buffer
	float depth = gbuffer_depth[screen_co].x;
	float3 wpos = unpack_position(float2(uv.x, 1.f - uv.y), depth);
	float3 albedo = albedo_roughness.rgb;
	float roughness = albedo_roughness.w;
	float3 normal = normal_metallic.xyz;
	float metallic = normal_metallic.w;

	// Do lighting
	float3 cpos = float3(inv_view[0][3], inv_view[1][3], inv_view[2][3]);
	float3 V = normalize(cpos - wpos);

	if (length(normal) == 0)		//TODO: Could be optimized by only marking pixels that need lighting, but that would require execute rays indirect
	{
		gOutput[DispatchRaysIndex().xy] = float4(skybox.SampleLevel(s0, SampleSphericalMap(-V), 0));
		return;
	}

<<<<<<< HEAD
	float3 lighting = ShadePixel(wpos, V, albedo, normal, roughness, metallic, rand_seed);
	nextRand(rand_seed);
	float3 reflection = DoReflection(wpos, V, normal, roughness, metallic, albedo, lighting, rand_seed);
=======
	float3 lighting = shade_pixel(wpos, V, albedo, metallic, roughness, normal, 0);
	float3 reflection = DoReflection(wpos, V, normal, roughness, metallic, albedo, lighting);
>>>>>>> master

	float3 flipped_N = normal;
	flipped_N.y *= -1;
	const float3 sampled_irradiance = irradiance_map.SampleLevel(s0, flipped_N, 0).xyz;

	const float3 F = F_SchlickRoughness(max(dot(normal, V), 0.0), metallic, albedo, roughness);
	float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

	float3 specular = (reflection.xyz) * F;
	float3 diffuse = albedo * sampled_irradiance;
	float3 ambient = (kD * diffuse + specular);

	gOutput[DispatchRaysIndex().xy] = float4(ambient + lighting, 1);

}

//Reflections

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
void ReflectionHit(inout ReflectionHitInfo payload, in MyAttributes attr)
{
	// Calculate the essentials
	const Offset offset = g_offsets[InstanceID()];
	const Material material = g_materials[offset.material_idx];
	const float index_offset = offset.idx_offset;
	const float vertex_offset = offset.vertex_offset;
	const float3x4 model_matrix = ObjectToWorld3x4();

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
	float2 uv = HitAttribute(float3(v0.uv, 0), float3(v1.uv, 0), float3(v2.uv, 0), attr).xy;
	uv.y = 1.0f - uv.y;

	float3 albedo = g_textures[material.albedo_id].SampleLevel(s0, uv, 0).xyz;
	float roughness = max(0.05, g_textures[material.roughness_id].SampleLevel(s0, uv, 0).x);
	float metal = g_textures[material.metalicness_id].SampleLevel(s0, uv, 0).x;

	//Direction & position
	const float3 hit_pos = HitWorldPosition();
	float3 V = normalize(payload.origin - hit_pos);

	//Normal mapping
	float3 normal = normalize(HitAttribute(v0.normal, v1.normal, v2.normal, attr));
	float3 tangent = HitAttribute(v0.tangent, v1.tangent, v2.tangent, attr);
	float3 bitangent = HitAttribute(v0.bitangent, v1.bitangent, v2.bitangent, attr);

	float3 N = normalize(mul(model_matrix, float4(normal, 0)));
	float3 T = normalize(mul(model_matrix, float4(tangent, 0)));
	float3 B = normalize(mul(model_matrix, float4(bitangent, 0)));
	float3x3 TBN = float3x3(T, B, N);

	float3 normal_t = (g_textures[material.normal_id].SampleLevel(s0, uv, 0).xyz) * 2.0 - float3(1.0, 1.0, 1.0);

	float3 fN = normalize(mul(normal_t, TBN));
	if (dot(fN, V) <= 0.0f) fN = -fN;

	//TODO: Reflections

	//Shading
<<<<<<< HEAD
	payload.color = ShadePixel(hit_pos, V, albedo, fN, roughness, metal, payload.seed);
=======
	float3 flipped_N = fN;
	flipped_N.y *= -1;
	const float3 sampled_irradiance = irradiance_map.SampleLevel(s0, flipped_N, 0).xyz;

	const float3 F = F_SchlickRoughness(max(dot(fN, V), 0.0), metal, albedo, roughness);
	float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - metal;

	float3 specular = (float3(0, 0, 0)) * F;
	float3 diffuse = albedo * sampled_irradiance;
	float3 ambient = (kD * diffuse + specular);

	float3 lighting = shade_pixel(hit_pos, V, albedo, metal, roughness, fN, 1);

	payload.color = ambient + lighting;
>>>>>>> master
}

//Reflection skybox

[shader("miss")]
void ReflectionMiss(inout ReflectionHitInfo payload)
{
	payload.color = skybox.SampleLevel(s0, SampleSphericalMap(WorldRayDirection()), 0);
}

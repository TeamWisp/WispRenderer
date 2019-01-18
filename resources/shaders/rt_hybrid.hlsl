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
	float3 color;
};

struct Material
{
	float idx_offset;
	float vertex_offset;
	float albedo_id;
	float normal_id;
	float roughness_id;
	float metalicness_id;
};

RWTexture2D<float4> gOutput : register(u0);
RaytracingAccelerationStructure Scene : register(t0, space0);
ByteAddressBuffer g_indices : register(t1);
StructuredBuffer<Light> lights : register(t2);
StructuredBuffer<Vertex> g_vertices : register(t3);
StructuredBuffer<Material> g_materials : register(t4);

Texture2D g_textures[20] : register(t6);
Texture2D gbuffer_albedo : register(t26);
Texture2D gbuffer_normal : register(t27);
Texture2D gbuffer_depth : register(t28);
Texture2D skybox : register(t5);
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

	float2 padding;
	float pbr_roughness;
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

// Retrieve hit world position.
float3 HitWorldPosition()
{
	return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

bool TraceShadowRay(float3 origin, float3 direction, float t_max)
{
	ShadowHitInfo payload = { false };

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

float3 TraceReflectionRay(float3 origin, float3 norm, float3 direction)
{
	const float epsilon = 5e-3;
	origin += norm * epsilon;

	ReflectionHitInfo payload = { origin, float3(0, 0, 1) };

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

float calc_attenuation(float r, float d)
{
	return 1.0f - smoothstep(r * 0, r, d);
}

float3 ShadeLight(float3 wpos, float3 V, float3 albedo, float3 normal, float roughness, float metal, Light light)
{
	uint tid = light.tid & 3;

	//Light direction (constant with directional, position dependent with other)
	float3 L = (lerp(light.pos - wpos, -light.dir, tid == light_type_directional));
	float light_dist = length(L);
	L /= light_dist;

	float range = light.radius;
	const float attenuation = calc_attenuation(range, light_dist);

	//Spot intensity (only used with spot; but always calculated)
	float min_cos = cos(light.angle);
	float max_cos = lerp(min_cos, 1, 0.5f);
	float cos_angle = dot(light.dir, -L);
	float spot_intensity = lerp(smoothstep(min_cos, max_cos, cos_angle), 1, tid != light_type_spot);

	// Calculate radiance
	const float3 radiance = (intensity * spot_intensity * light.color) * attenuation;

	float3 lighting = BRDF(L, V, normal, metal, roughness, albedo, radiance, light.color);

	// Check if pixel is shaded
	float epsilon = 0.005; // Hard-coded; use depth buffer to get depth value in linear space and use that to determine the epsilon (to minimize precision errors)
	float3 origin = wpos + normal * 0.005;
	float t_max = lerp(light_dist, 10000.0, tid == light_type_directional);
	bool is_shadow = TraceShadowRay(origin, L, t_max);

	lighting = lerp(lighting, float3(0, 0, 0), is_shadow);

	return lighting;
}

float3 ShadePixel(float3 pos, float3 V, float3 albedo, float3 normal, float roughness, float metal)
{
	uint light_count = lights[0].tid >> 2;	//Light count is stored in 30 upper-bits of first light

	float ambient = 0.1f;
	float3 res = float3(ambient, ambient, ambient);

	for (uint i = 0; i < light_count; i++)
	{
		res += ShadeLight(pos, V, albedo, normal, roughness, metal, lights[i]);
	}

	return res * albedo;
}


float3 ReflectRay(float3 v1, float3 v2)
{
	return (v2 * ((2.f * dot(v1, v2))) - v1);
}

float3 DoReflection(float3 wpos, float3 V, float3 normal, float roughness, float metallic, float3 albedo, float3 lighting)
{

	// Calculate ray info

	float3 reflected = ReflectRay(V, normal);

	// Shoot reflection ray
	
	float3 reflection = TraceReflectionRay(wpos, normal, reflected);

	// Calculate reflection combined with fresnel

	const float3 F = F_SchlickRoughness(max(dot(normal, V), 0.0), metallic, albedo, roughness);
	const float3 kS = F;
	float3 kD = 1.0 - kS;
	kD *= 1.0 - metallic;

	const float3 specular = F * reflection;

	return specular + lighting;
}

#define M_PI 3.14159265358979

float2 VectorToLatLong(float3 dir)
{
	float3 p = normalize(dir);

	// atan2_WAR is a work-around due to an apparent compiler bug in atan2
	float u = (1.f + atan2(p.x, -p.z) / M_PI) * 0.5f;
	float v = acos(p.y*-1) / M_PI;
	return float2(u, v);
}

float3 SampleSkybox(float3 dir)
{
	// Load some information about our lightprobe texture
	float2 dims;
	skybox.GetDimensions(dims.x, dims.y);

	// Convert our ray direction to a (u,v) coordinate
	float2 uv = VectorToLatLong(dir);
	return skybox[uint2(uv * dims)].rgb;
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
	float3 normal = normal_metallic.xyz;
	float metallic = normal_metallic.w;

	// Do lighting
	float3 cpos = float3(inv_view[0][3], inv_view[1][3], inv_view[2][3]);
	float3 V = normalize(cpos - wpos);

	if (length(normal) == 0)		//TODO: Could be optimized by only marking pixels that need lighting, but that would require execute rays indirect
	{
		gOutput[DispatchRaysIndex().xy] = float4(SampleSkybox(-V), 1);
		return;
	}

	float3 lighting = ShadePixel(wpos, V, albedo, normal, roughness, metallic);
	float3 reflection = DoReflection(wpos, V, normal, roughness, metallic, albedo, lighting);

	gOutput[DispatchRaysIndex().xy] = float4(reflection, 1);

}

//Shadows

[shader("closesthit")]
void ShadowHit(inout ShadowHitInfo payload, in MyAttributes attr)
{
	payload.shadow_hit = true;
}

[shader("miss")]
void ShadowMiss(inout ShadowHitInfo payload)
{
	payload.shadow_hit = false;
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
	const Material material = g_materials[InstanceID()];
	const float index_offset = material.idx_offset;
	const float vertex_offset = material.vertex_offset;
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
	float3 color = HitAttribute(v0.color, v1.color, v2.color, attr);
	float2 uv = HitAttribute(float3(v0.uv, 0), float3(v1.uv, 0), float3(v2.uv, 0), attr).xy;
	float3 albedo = pow(g_textures[material.albedo_id].SampleLevel(s0, uv, 0).xyz, 2.2);
	float roughness = g_textures[material.roughness_id].SampleLevel(s0, uv, 0).x;
	float metal = g_textures[material.metalicness_id].SampleLevel(s0, uv, 0).x;

	albedo = lerp(albedo, color, length(color) != 0);

	//Direction & position

	const float3 hit_pos = HitWorldPosition();
	float3 V = normalize(payload.origin - hit_pos);

	//Normal mapping
	float3 normal = normalize(HitAttribute(v0.normal, v1.normal, v2.normal, attr));
	float3 tangent = HitAttribute(v0.tangent, v1.tangent, v2.tangent, attr);
	float3 bitangent = HitAttribute(v0.bitangent, v1.bitangent, v2.bitangent, attr);

	float3 N = normalize(mul(model_matrix, float4(normal, 0)).xyz);
	float3 T = normalize(mul(model_matrix, float4(tangent, 0)).xyz);
	float3 B = normalize(mul(ObjectToWorld3x4(), float4(bitangent, 0)));
	float3x3 TBN = float3x3(T, B, N);

	float3 normal_t = (g_textures[material.normal_id].SampleLevel(s0, uv, 0).xyz) * 2.0 - float3(1.0, 1.0, 1.0);

	float3 fN = normalize(mul(normal_t, TBN));
	if (dot(fN, V) <= 0.0f) fN = -fN;

	//TODO: Reflections

	//Shading
	payload.color = ShadePixel(hit_pos, V, albedo, fN, roughness, metal);
}

//Reflection skybox

[shader("miss")]
void ReflectionMiss(inout ReflectionHitInfo payload)
{
	payload.color = SampleSkybox(WorldRayDirection());
}
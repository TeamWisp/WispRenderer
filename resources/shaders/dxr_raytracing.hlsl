#ifndef __DXR_RAYTRACING_HLSL__
#define __DXR_RAYTRACING_HLSL__

#define LIGHTS_REGISTER register(t2)
#include "rand_util.hlsl"
#include "pbr_util.hlsl"
#include "lighting.hlsl"
#include "material_util.hlsl"

// Definitions for: 
// - Vertex, Material, Offset
// - Ray, RayCone, ReflectionHitInfo
#include "dxr_structs.hlsl"

// Definitions for: 
// - HitWorldPosition, Load3x32BitIndices, unpack_position, HitAttribute
#include "dxr_functions.hlsl"

RWTexture2D<float4> gOutput : register(u0);
ByteAddressBuffer g_indices : register(t1);
StructuredBuffer<Vertex> g_vertices : register(t3);
StructuredBuffer<Material> g_materials : register(t4);
StructuredBuffer<Offset> g_offsets : register(t5);

TextureCube skybox : register(t6);
Texture2D brdf_lut : register(t7);
TextureCube irradiance_map : register(t8);
Texture2D g_textures[1000] : register(t9);
SamplerState s0 : register(s0);
SamplerState point_sampler : register(s1);

typedef BuiltInTriangleIntersectionAttributes MyAttributes;

#include "dxr_pathtracer_functions.hlsl"

cbuffer CameraProperties : register(b0)
{
	float4x4 view;
	float4x4 inv_projection_view;
	float3 camera_position;
	float padding;

	float focal_radius;
	float focal_len;
	float frame_idx;
	float intensity;
};

inline Ray GenerateCameraRay(uint2 index, in float3 cameraPosition, in float4x4 projectionToWorld, in float2 offset, unsigned int seed)
{
#ifdef DEPTH_OF_FIELD
	float2 pixelOff = float2(nextRand(seed), nextRand(seed));  // Random offset in pixel to reduce floating point error.

	float3 cameraU = float3(1, 0, 0);
	float3 cameraV = float3(0, 1, 0);
	float3 cameraW = float3(0, 0, 1);

	float2 xy = (index + offset + pixelOff) + 0.5f; // center in the middle of the pixel.
	float2 screenPos = xy / DispatchRaysDimensions().xy;

	// Invert Y for DirectX-style coordinates.
	screenPos.y = -screenPos.y;

	// Unproject the pixel coordinate into a world positon.
	float4 world = mul(float4(screenPos, 0, 1), projectionToWorld);
	world.xyz = world.x * cameraU + world.y * cameraV + cameraW;
	world.xyz /= 1;

	float2 pixelCenter = (index + offset) / DispatchRaysDimensions().xy;            // Pixel ID -> [0..1] over screen
	float2 ndc = float2(2, -2) * pixelCenter + float2(-1, 1);             // Convert to [-1..1]
	float3 rayDir = ndc.x * cameraU + ndc.y * cameraV + cameraW;  // Ray to point on near plane
	rayDir /= 1;

	float focallen = focal_len;
	float lensrad = focal_len / (2.0f * 16);
	float3 focalPt = cameraPosition + focallen * world;

	float2 rngLens = float2(6.2831853f * nextRand(seed), lensrad * nextRand(seed));
	float2 lensPos = float2(cos(rngLens.x) * rngLens.y, sin(rngLens.x) * rngLens.y);

	//lensPos = mul(float4(lensPos, 0, 0), projectionToWorld);

	Ray ray;
	ray.origin = cameraPosition + float3(lensPos, 0);
	ray.direction = normalize(focalPt.xyz - ray.origin);
#else
	float2 xy = (index + offset) + 0.5f; // center in the middle of the pixel.
	float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

	// Invert Y for DirectX-style coordinates.
	screenPos.y = -screenPos.y;

	// Unproject the pixel coordinate into a world positon.
	float4 world = mul(float4(screenPos, 0, 1), projectionToWorld);
	world.xyz /= world.w;

	Ray ray;
	ray.origin = cameraPosition;
	ray.direction = normalize(world.xyz - ray.origin);

	return ray;
#endif
}

[shader("raygeneration")]
void RaygenEntry()
{
	uint rand_seed = initRand(DispatchRaysIndex().x + DispatchRaysIndex().y * DispatchRaysDimensions().x, frame_idx);

#ifdef FOUR_X_AA
	Ray a = GenerateCameraRay(DispatchRaysIndex().xy, camera_position, inv_projection_view, float2(0.5, 0), rand_seed);
	Ray b = GenerateCameraRay(DispatchRaysIndex().xy, camera_position, inv_projection_view, float2(-0.5, 0), rand_seed);
	Ray c = GenerateCameraRay(DispatchRaysIndex().xy, camera_position, inv_projection_view, float2(0.0, 0.5), rand_seed);
	Ray d = GenerateCameraRay(DispatchRaysIndex().xy, camera_position, inv_projection_view, float2(0.0, -0.5), rand_seed);

	float3 result_a = TraceColorRay(a.origin, a.direction, 0, rand_seed);
	nextRand(rand_seed);
	float3 result_b = TraceColorRay(b.origin, b.direction, 0, rand_seed);
	nextRand(rand_seed);
	float3 result_c = TraceColorRay(c.origin, c.direction, 0, rand_seed);
	nextRand(rand_seed);
	float3 result_d = TraceColorRay(d.origin, d.direction, 0, rand_seed);

	float3 result = (result_a + result_b + result_c + result_d) / 4;
#else
	Ray ray = GenerateCameraRay(DispatchRaysIndex().xy, camera_position, inv_projection_view, float2(0, 0), rand_seed);
	float3 result = TraceColorRay(ray.origin, ray.direction, 0, rand_seed);
#endif

	if (any(isnan(result)))
	{
		result = float3(1000, 0, 0);
	}

	float4 prev = gOutput[DispatchRaysIndex().xy];

	float4 color = (frame_idx * prev + float4(result, 1)) / (frame_idx + 1); // accumulate

	gOutput[DispatchRaysIndex().xy] = color;
}

[shader("miss")]
void MissEntry(inout FullRTHitInfo payload)
{
	payload.color = skybox.SampleLevel(s0, WorldRayDirection(), 0).rgb;
}

[shader("closesthit")]
void ClosestHitEntry(inout FullRTHitInfo payload, in MyAttributes attr)
{
	// Calculate the essentials
	const Offset offset = g_offsets[InstanceID()];
	const Material material = g_materials[offset.material_idx];
	const float3 hit_pos = HitWorldPosition();
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

	// Variables
	const float3 V = normalize(payload.origin - hit_pos);

	// Calculate actual "fragment" attributes.
	const float3 frag_pos = HitAttribute(v0.pos, v1.pos, v2.pos, attr);
	const float3 normal = normalize(HitAttribute(v0.normal, v1.normal, v2.normal, attr));
	const float3 tangent = HitAttribute(v0.tangent, v1.tangent, v2.tangent, attr);
	const float3 bitangent = HitAttribute(v0.bitangent, v1.bitangent, v2.bitangent, attr);

	float2 uv = HitAttribute(float3(v0.uv, 0), float3(v1.uv, 0), float3(v2.uv, 0), attr).xy;
	uv.y = 1.0f - uv.y;

	float mip_level = payload.depth;

	OutputMaterialData output_data = InterpretMaterialDataRT(material.data,
		g_textures[material.albedo_id],
		g_textures[material.normal_id],
		g_textures[material.roughness_id],
		g_textures[material.metalicness_id],
		g_textures[material.emissive_id],
		g_textures[material.ao_id],
		mip_level,
		s0,
		uv);

	float3 albedo = output_data.albedo;
	float roughness = output_data.roughness;
	float metal = output_data.metallic;
	float3 emissive = output_data.emissive;
	float ao = output_data.ao;

	float3 N = 0;
	float3 fN = 0;
	if (payload.depth > 0)
	{
		N = normalize(mul(ObjectToWorld3x4(), float4(normal, 0)));
		fN = N;
	}
	else
	{
		N = 0;
		fN = CalcPeturbedNormal(normal, output_data.normal, tangent, bitangent, V, N);
	}

	nextRand(payload.seed);
	payload.color = ggxIndirect(hit_pos, fN, N, V, albedo, metal, roughness, ao, payload.seed, payload.depth + 1);
	payload.color += ggxDirect(hit_pos, fN, N, V, albedo, metal, roughness, payload.seed, payload.depth + 1);
	payload.color += emissive;
}

[shader("closesthit")]
void ShadowClosestHitEntry(inout ShadowHitInfo hit, MyAttributes bary)
{
	hit.is_hit = true;
}

[shader("miss")]
void ShadowMissEntry(inout ShadowHitInfo hit : SV_RayPayload)
{
	hit.is_hit = false;
}

#endif //__DXR_RAYTRACING_HLSL__

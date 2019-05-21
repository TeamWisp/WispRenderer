#ifndef __RT_HYBRID__
#define __RT_HYBRID__

#define LIGHTS_REGISTER register(t2)
#include "util.hlsl"
#include "pbr_util.hlsl"
#include "material_util.hlsl"
#include "lighting.hlsl"
#include "rt_texture_lod.hlsl"
#include "rt_structs.hlsl"

RWTexture2D<float4> output_refl_shadow : register(u0); // xyz: reflection, a: shadow factor
ByteAddressBuffer g_indices : register(t1);
StructuredBuffer<Vertex> g_vertices : register(t3);
StructuredBuffer<Material> g_materials : register(t4);
StructuredBuffer<Offset> g_offsets : register(t5);

Texture2D g_textures[1000] : register(t10);
Texture2D gbuffer_albedo : register(t1010);
Texture2D gbuffer_normal : register(t1011);
Texture2D gbuffer_depth : register(t1012);
Texture2D skybox : register(t6);
Texture2D brdf_lut : register(t8);
TextureCube irradiance_map : register(t9);
SamplerState s0 : register(s0);

typedef BuiltInTriangleIntersectionAttributes MyAttributes;

cbuffer CameraProperties : register(b0)
{
	float4x4 inv_view;
	float4x4 inv_projection;
	float4x4 inv_vp;

	float2 padding;
	float frame_idx;
	float intensity;
};

#define M_PI 3.14159265358979

#include "rt_reflection_shaders.hlsl"
#include "rt_shadow_shaders.hlsl"

[shader("raygeneration")]
void RaygenEntry()
{
	//uint rand_seed = initRand(DispatchRaysIndex().x + DispatchRaysIndex().y * DispatchRaysDimensions().x, frame_idx);
	uint rand_seed = initRand(DispatchRaysIndex().x % TILE_SIZE_2D, DispatchRaysIndex().y % TILE_SIZE_2D);

	// Texture UV coordinates [0, 1]
	float2 uv = float2(DispatchRaysIndex().xy + 0.5f) / float2(DispatchRaysDimensions().xy);

	// Screen coordinates [0, resolution] (inverted y)
	int2 screen_co = DispatchRaysIndex().xy;

	// Get g-buffer information
	float4 albedo_roughness = gbuffer_albedo.SampleLevel(s0, uv, 0);
	float4 normal_metallic = gbuffer_normal.SampleLevel(s0, uv, 0);

	// Unpack G-Buffer
	float depth = gbuffer_depth.SampleLevel(s0, uv, 0).x;
	float3 wpos = unpack_position(float2(uv.x, 1.f - uv.y), depth, inv_vp);
	float3 albedo = albedo_roughness.rgb;
	float roughness = albedo_roughness.w;
	float3 normal = normalize(normal_metallic.xyz);
	float metallic = normal_metallic.w;

	// Do lighting
	float3 cpos = float3(inv_view[0][3], inv_view[1][3], inv_view[2][3]);
	float3 V = normalize(cpos - wpos);

	if (length(normal) == 0)		//TODO: Could be optimized by only marking pixels that need lighting, but that would require execute rays indirect
	{
		// A value of 1 in the output buffer, means that there is shadow
		// So, the far plane pixels are set to 0
		output_refl_shadow[DispatchRaysIndex().xy] = float4(0, 0, 0, 0);
		return;
	}

	normal = lerp(normal, -normal, dot(normal, V) < 0);

	// Describe the surface for mip level generation
	SurfaceHit sfhit;
	sfhit.pos = wpos;
	sfhit.normal = normal;
	sfhit.dist = length(cpos - wpos);
	sfhit.surface_spread_angle = ComputeSurfaceSpreadAngle(gbuffer_depth, gbuffer_normal, inv_vp, wpos, normal);

	// Compute the initial ray cone from the gbuffers.
 	RayCone cone = ComputeRayConeFromGBuffer(sfhit, 1.39626, DispatchRaysDimensions().y);
	
	// Get shadow factor
	float shadow_result = DoShadowAllLights(wpos, normal, 0, rand_seed);
	//float shadow_result = 1.0f;

	// Get reflection result
	float3 reflection_result = clamp(DoReflection(wpos, V, normal, rand_seed, depth, cone), 0, 100000);

	// xyz: reflection, a: shadow factor
	output_refl_shadow[DispatchRaysIndex().xy] = float4(reflection_result.xyz, shadow_result);
}


#endif //__RT_HYBRID__
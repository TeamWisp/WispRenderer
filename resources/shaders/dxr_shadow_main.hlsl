#ifndef __DXR_SHADOW_MAIN_HLSL__
#define __DXR_SHADOW_MAIN_HLSL__

#define LIGHTS_REGISTER register(t2)
#include "rand_util.hlsl"
#include "pbr_util.hlsl"
#include "math.hlsl"
#include "material_util.hlsl"
#include "lighting.hlsl"
#include "dxr_texture_lod.hlsl"
#include "dxr_global.hlsl"

// Definitions for: 
// - Vertex, Material, Offset
// - Ray, RayCone, ReflectionHitInfo
#include "dxr_structs.hlsl"

// Definitions for: 
// - HitWorldPosition, Load3x32BitIndices, unpack_position, HitAttribute
#include "dxr_functions.hlsl"

RWTexture2D<float4> output_refl_shadow : register(u0); // xyz: reflection, a: shadow factor
ByteAddressBuffer g_indices : register(t1);
StructuredBuffer<Vertex> g_vertices : register(t3);
StructuredBuffer<Material> g_materials : register(t4);
StructuredBuffer<Offset> g_offsets : register(t5);

Texture2D g_textures[1000] : register(t10);
Texture2D gbuffer_albedo : register(t1010);
Texture2D gbuffer_normal : register(t1011);
Texture2D gbuffer_depth : register(t1012);
TextureCube skybox : register(t6);
Texture2D brdf_lut : register(t8);
TextureCube irradiance_map : register(t9);
SamplerState s0 : register(s0);

typedef BuiltInTriangleIntersectionAttributes Attributes;

cbuffer CameraProperties : register(b0)
{
	float4x4 inv_view;
	float4x4 inv_projection;
	float4x4 inv_vp;

	float frame_idx;
	float intensity;
	float epsilon;
	unsigned int shadow_sample_count;
};

#include "dxr_shadow_functions.hlsl"
#include "dxr_shadow_entries.hlsl"

[shader("raygeneration")]
void ShadowRaygenEntry()
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
	float3 wpos = unpack_position(float2(uv.x, 1.f - uv.y), depth, inv_vp);
	float3 normal = normal_metallic.xyz;

	// Do lighting
	float3 cpos = float3(inv_view[0][3], inv_view[1][3], inv_view[2][3]);
	float3 V = normalize(cpos - wpos);

	if (length(normal) == 0)		//TODO: Could be optimized by only marking pixels that need lighting, but that would require execute rays indirect
	{
		// A value of 1 in the output buffer, means that there is shadow
		// So, the far plane pixels are set to 0
		output_refl_shadow[screen_co] = float4(1, 1, 1, 1);
		return;
	}

	wpos += normal * epsilon;
	// Get shadow factor
	float4 shadow_result = DoShadowAllLights(wpos, V, normal, normal_metallic.w, albedo_roughness.w, albedo_roughness.xyz, shadow_sample_count, 0, CALLINGPASS_SHADOWS, rand_seed);

	// xyz: reflection, a: shadow factor
	output_refl_shadow[screen_co] = shadow_result;
}

#endif //__DXR_SHADOW_MAIN_HLSL__
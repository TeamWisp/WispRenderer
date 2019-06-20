/*!
 * Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __DXR_REFLECTION_MAIN_HLSL__
#define __DXR_REFLECTION_MAIN_HLSL__

#define LIGHTS_REGISTER register(t2)
#include "rand_util.hlsl"
#include "pbr_util.hlsl"
#include "material_util.hlsl"
#include "lighting.hlsl"
#include "dxr_texture_lod.hlsl"

// Definitions for: 
// - Vertex, Material, Offset
// - Ray, RayCone, ReflectionHitInfo
#include "dxr_structs.hlsl"

// Definitions for: 
// - HitWorldPosition, Load3x32BitIndices, unpack_position, HitAttribute
#include "dxr_functions.hlsl"

RWTexture2D<float4> output_reflection : register(u0); // rgb: reflection, a: pdf
RWTexture2D<unorm float> output_shadow : register(u1); // r: shadow factor
RWTexture2D<float4> output_dir_t_buffer : register(u2); // xyz: direction, w: hitT; dirT to calculate hit position
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

#include "dxr_reflection_functions.hlsl"
#include "dxr_reflection_entries.hlsl"
#include "dxr_shadow_entries.hlsl"

[shader("raygeneration")]
void ReflectionRaygenEntry()
{
	uint rand_seed = initRand(DispatchRaysIndex().x + DispatchRaysIndex().y * DispatchRaysDimensions().x, frame_idx);

	// Texture UV coordinates [0, 1]
	float2 uv = float2(DispatchRaysIndex().xy + 0.5) / float2(DispatchRaysDimensions().xy);

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
	float3 normal = normal_metallic.xyz;
	float metallic = normal_metallic.w;

	// Do lighting
	float3 cpos = float3(inv_view[0][3], inv_view[1][3], inv_view[2][3]);
	float3 V = normalize(cpos - wpos);

	if (length(normal) == 0)		//TODO: Could be optimized by only marking pixels that need lighting, but that would require execute rays indirect
	{
		// A value of 1 in the output buffer, means that there is shadow
		// So, the far plane pixels are set to 0
		output_reflection[screen_co] = float4(0, 0, 0, 0);
		output_shadow[screen_co] = 0;
		output_dir_t_buffer[screen_co] = float4(0, 0, 0, 0);
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

	// Get reflection result
	float4 dir_t = float4(0, 0, 0, 0);
	float4 reflection_result = min(DoReflection(wpos, V, normal, rand_seed, 0, roughness, metallic, cone, dir_t), 10000);

	// xyz: reflection, a: shadow factor
	output_reflection[screen_co] = reflection_result;
	output_dir_t_buffer[screen_co] = dir_t;
}

#endif //__DXR_REFLECTION_MAIN_HLSL__
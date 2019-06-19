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
#ifndef __DXR_PATHTRACER_MAIN_HLSL__
#define __DXR_PATHTRACER_MAIN_HLSL__

#define LIGHTS_REGISTER register(t2)
#include "rand_util.hlsl"
#include "pbr_util.hlsl"
#include "material_util.hlsl"
#include "lighting.hlsl"
#include "dxr_texture_lod.hlsl"

// Definitions for: 
// - Vertex, Material, Offset
// - Ray, RayCone, ReflectionHitInfo, etc
#include "dxr_structs.hlsl"

// Definitions for: 
// - HitWorldPosition, Load3x32BitIndices, unpack_position, HitAttribute
#include "dxr_functions.hlsl"

RWTexture2D<float4> output : register(u0); // xyz: reflection, a: shadow factor
ByteAddressBuffer g_indices : register(t1);
StructuredBuffer<Vertex> g_vertices : register(t3);
StructuredBuffer<Material> g_materials : register(t4);
StructuredBuffer<Offset> g_offsets : register(t5);

Texture2D g_textures[1000] : register(t10);
Texture2D gbuffer_albedo : register(t1010);
Texture2D gbuffer_normal : register(t1011);
Texture2D gbuffer_emissive : register(t1012);
Texture2D gbuffer_depth : register(t1013);
TextureCube skybox : register(t6);
TextureCube irradiance_map : register(t9);
SamplerState s0 : register(s0);

typedef BuiltInTriangleIntersectionAttributes Attributes;

cbuffer CameraProperties : register(b0)
{
	float4x4 inv_view;
	float4x4 inv_projection;
	float4x4 inv_vp;

	float frame_idx;
	float3 padding;
};

#include "dxr_pathtracer_functions.hlsl"
#include "dxr_pathtracer_entries.hlsl"

[shader("raygeneration")]
void RaygenEntry()
{
	uint rand_seed = initRand((DispatchRaysIndex().x + DispatchRaysIndex().y * DispatchRaysDimensions().x), frame_idx);

	// Texture UV coordinates [0, 1]
	float2 uv = float2(DispatchRaysIndex().xy) / float2(DispatchRaysDimensions().xy - 1);

	// Screen coordinates [0, resolution] (inverted y)
	int2 screen_co = DispatchRaysIndex().xy;

	// Get g-buffer information
	float4 albedo_roughness = gbuffer_albedo[screen_co];
	float4 normal_metallic = gbuffer_normal[screen_co];
	float4 emissive_ao = gbuffer_emissive[screen_co];

	// Unpack G-Buffer
	float depth = gbuffer_depth[screen_co].x;
	float3 albedo = albedo_roughness.rgb;
	float3 wpos = unpack_position(float2(uv.x, 1.f - uv.y), depth, inv_vp);
	float3 normal = normalize(normal_metallic.xyz);
	float metallic = normal_metallic.w;
	float roughness = albedo_roughness.w;
	float3 emissive = emissive_ao.xyz;
	float ao = emissive_ao.w;

	// Do lighting
	float3 cpos = float3(inv_view[0][3], inv_view[1][3], inv_view[2][3]);
	float3 V = normalize(cpos - wpos);

	float3 result = float3(0, 0, 0);

	SurfaceHit sfhit;
	sfhit.pos = wpos;
	sfhit.normal = normal;
	sfhit.dist = length(cpos - wpos);
	sfhit.surface_spread_angle = ComputeSurfaceSpreadAngle(gbuffer_depth, gbuffer_normal, inv_vp, wpos, normal);

	// Compute the initial ray cone from the gbuffers.
 	RayCone cone = ComputeRayConeFromGBuffer(sfhit, 1.39626, DispatchRaysDimensions().y);

	nextRand(rand_seed);
	const float3 rand_dir = getCosHemisphereSample(rand_seed, normal);
	const float cos_theta = cos(dot(rand_dir, normal));
	result = TraceColorRayCone(wpos + (EPSILON * normal), rand_dir, 0, rand_seed, cone);

	if (any(isnan(result)))
	{
		result = 0;
	}

	result = clamp(result, 0, 100);
	
	output[DispatchRaysIndex().xy] = float4(result, 1);
}

#endif //__DXR_PATHTRACER_MAIN_HLSL__

#ifndef __SHADER_NAME_HLSL__
#define __SHADER_NAME_HLSL__

////////////////////////////
// INCLUDES
////////////////////////////
#define LIGHTS_REGISTER register(t2)
#include "util.hlsl"
#include "pbr_util.hlsl"
#include "material_util.hlsl"
#include "lighting.hlsl"
#include "dxr_texture_lod.hlsl"
#include "dxr_global.hlsl"

// Definitions for: 
// - Vertex, Material, Offset
// - Ray, RayCone, ReflectionHitInfo, etc
#include "dxr_structs.hlsl"

// Definitions for: 
// - HitWorldPosition, Load3x32BitIndices, unpack_position, HitAttribute, etc
#include "dxr_functions.hlsl"

////////////////////////////
// SHADER REGISTRIES
////////////////////////////
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

////////////////////////////
// CONSTANT BUFFERS
////////////////////////////

/*
*/

////////////////////////////
// MODULAR SHADER INCLUDES
////////////////////////////
#include "dxr_shader_template_functions.hlsl" //Code that includes functions used by the shader, i.e. TraceRay functions
#include "dxr_shader_template_entries.hlsl" //Code that includes hit, miss, anyhit shaders

[shader("raygeneration")]
void RaygenEntry()
{
}

#endif //__SHADER_NAME_HLSL__
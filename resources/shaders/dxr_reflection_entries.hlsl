#ifndef __DXR_REFLECTION_ENTRIES_HLSL__
#define __DXR_REFLECTION_ENTRIES_HLSL__

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

//Reflections
[shader("closesthit")]
void ReflectionHit(inout ReflectionHitInfo payload, in Attributes attr)
{
	// Calculate the essentials
	const Offset offset = g_offsets[InstanceID()];
	const Material material = g_materials[offset.material_idx];
	const float3 hit_pos = HitWorldPosition();
	const float index_offset = offset.idx_offset;
	const float vertex_offset = offset.vertex_offset;
	const float3x4 model_matrix = ObjectToWorld3x4();

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

	//Direction & position
	float3 V = normalize(payload.origin - hit_pos);

	//Normal mapping
	const float3 frag_pos = HitAttribute(v0.pos, v1.pos, v2.pos, attr);
	float3 normal = normalize(HitAttribute(v0.normal, v1.normal, v2.normal, attr));
	float3 tangent = HitAttribute(v0.tangent, v1.tangent, v2.tangent, attr);
	float3 bitangent = HitAttribute(v0.bitangent, v1.bitangent, v2.bitangent, attr);

	//Get data from VBO
	float2 uv = HitAttribute(float3(v0.uv, 0), float3(v1.uv, 0), float3(v2.uv, 0), attr).xy;
	uv.y = 1.0f - uv.y;

	// Propogate the ray cone
	payload.cone = Propagate(payload.cone, 0, length(payload.origin - hit_pos));

	// Calculate the texture LOD level 
	float mip_level = ComputeTextureLOD(
		payload.cone,
		V,
		normalize(mul(model_matrix, float4(normal, 0))),
		mul(model_matrix, float4(v0.pos, 1)),
		mul(model_matrix, float4(v1.pos, 1)),
		mul(model_matrix, float4(v2.pos, 1)),
		v0.uv,
		v1.uv,
		v2.uv,
		g_textures[material.albedo_id]);

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

	float3 N = normalize(mul(model_matrix, float4(-normal, 0)));
	float3 T = normalize(mul(model_matrix, float4(tangent, 0)));
#define CALC_B
#ifndef CALC_B
	const float3 B = normalize(mul(ObjectToWorld3x4(), float4(bitangent, 0)));
#else
	T = normalize(T - dot(T, N) * N);
	float3 B = cross(N, T);
#endif
	float3x3 TBN = float3x3(T, B, N);

	float3 fN = normalize(mul(output_data.normal, TBN));
	fN = lerp(fN, -fN, dot(fN, V) < 0);

	//Shading
	float3 flipped_N = fN;
	flipped_N.y *= -1;
	const float3 sampled_irradiance = irradiance_map.SampleLevel(s0, flipped_N, 0).xyz;

	// TODO: reflections in reflections

	const float3 F = F_SchlickRoughness(max(dot(fN, V), 0.0), metal, albedo, roughness);
	float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - metal;

	const float2 sampled_brdf = brdf_lut.SampleLevel(s0, float2(max(dot(fN, V), 0.01f), roughness), 0).rg;


	//Reflection in reflections
	float3 reflection = DoReflection(hit_pos, V, fN, payload.seed, payload.depth + 1, payload.cone);

	//Lighting
	#undef SOFT_SHADOWS
	float3 lighting = shade_pixel(hit_pos, V, albedo, metal, roughness, emissive, fN, payload.seed, shadow_sample_count, payload.depth + 1, CALLINGPASS_REFLECTIONS);
	#define SOFT_SHADOWS

	float3 specular = reflection * (kS * sampled_brdf.x + sampled_brdf.y);
	float3 diffuse = albedo * sampled_irradiance;
	float3 ambient = (kD * diffuse + specular) * ao;

	// Output the final reflections here
	payload.color = ambient + lighting;
}

//Reflection skybox
[shader("miss")]
void ReflectionMiss(inout ReflectionHitInfo payload)
{
	payload.color = skybox.SampleLevel(s0, WorldRayDirection(), 0);
}

[shader("anyhit")]
void ReflectionAnyHit(inout ReflectionHitInfo payload, in Attributes attr)
{
#ifndef FALLBACK
	// Calculate the essentials
	const Offset offset = g_offsets[InstanceID()];
	const Material material = g_materials[offset.material_idx];
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

	//Get data from VBO
	float2 uv = HitAttribute(float3(v0.uv, 0), float3(v1.uv, 0), float3(v2.uv, 0), attr).xy;
	uv.y = 1.0f - uv.y;

	OutputMaterialData output_data = InterpretMaterialDataRT(material.data,
		g_textures[material.albedo_id],
		g_textures[material.normal_id],
		g_textures[material.roughness_id],
		g_textures[material.metalicness_id],
		g_textures[material.emissive_id],
		g_textures[material.ao_id],
		0,
		s0,
		uv);

	float alpha = output_data.alpha;

	if (alpha < 0.5f)
	{
		IgnoreHit();
	}
	else
	{
		AcceptHitAndEndSearch();
	}
#else
	payload.color = float3(0.0f, 0.0f, 0.0f);
#endif
}

#endif //__DXR_REFLECTION_ENTRIES_HLSL__
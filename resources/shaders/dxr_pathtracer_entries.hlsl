#ifndef __DXR_PATHTRACER_ENTRIES_HLSL__
#define __DXR_PATHTRACER_ENTRIES_HLSL__

#include "dxr_shadow_entries.hlsl"

//Reflections
[shader("closesthit")]
void ReflectionHit(inout PathTracingHitInfoCone payload, in Attributes attr)
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

	float mip_level = payload.depth + 1;

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

#ifdef NO_PATH_TRACED_NORMALS
	float3 N = normalize(mul(ObjectToWorld3x4(), float4(normal, 0)));
	float3 fN = N;
#else
	float3 N = 0;
	float3 fN = CalcPeturbedNormal(normal, output_data.normal, tangent, bitangent, V, N);
#endif

	// #################### GGX #####################
	nextRand(payload.seed);
	payload.color = ggxIndirect(hit_pos, fN, N, V, albedo, metal, roughness, ao, payload.seed, payload.depth + 1, true, payload.cone);
	payload.color += ggxDirect(hit_pos, fN, N, V, albedo, metal, roughness, payload.seed, payload.depth + 1);
	payload.color += emissive;
}

//Reflection skybox
[shader("miss")]
void ReflectionMiss(inout PathTracingHitInfoCone payload)
{
	payload.color = skybox.SampleLevel(s0, WorldRayDirection(), 0).rgb;
}

#endif //__DXR_PATHTRACER_ENTRIES_HLSL__

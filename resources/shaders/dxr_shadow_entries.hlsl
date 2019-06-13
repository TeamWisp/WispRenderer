#ifndef __DXR_SHADOW_ENTRIES_HLSL__
#define __DXR_SHADOW_ENTRIES_HLSL__

#include "pbr_util.hlsl"
#include "material_util.hlsl"

// Definitions for: 
// - Vertex, Material, Offset
// - Ray, RayCone, ReflectionHitInfo
#include "dxr_structs.hlsl"

[shader("closesthit")]
void ShadowClosestHitEntry(inout ShadowHitInfo hit, Attributes bary)
{
	hit.ray_power = 0.1f;
	hit.is_hit = true;
}

[shader("miss")]
void ShadowMissEntry(inout ShadowHitInfo hit : SV_RayPayload)
{
	hit.is_hit = false;
}

[shader("anyhit")]
void ShadowAnyHitEntry(inout ShadowHitInfo hit, Attributes attr)
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

	if (material.data.use_alpha_masking == 1.0f)
	{
		float alpha = output_data.alpha;

		if (alpha <= 0.5f)
		{
			IgnoreHit();
		}

		hit.ray_power -= (1.0f / 3.0f);

		if (hit.ray_power <= 0.02f)
		{
			AcceptHitAndEndSearch();
		}
		else
		{
			IgnoreHit();
		}
	}
	else
	{
		AcceptHitAndEndSearch();
	}

#else
	hit.is_hit = false;
#endif
}

#endif //__DXR_SHADOW_ENTRIES_HLSL__
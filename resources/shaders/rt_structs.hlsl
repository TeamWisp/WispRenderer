#ifndef __RT_STRUCTS__
#define __RT_STRUCTS__

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
	uint albedo_id;
	uint normal_id;
	uint roughness_id;
	uint metalicness_id;
	uint emissive_id;
	uint ao_id;
	float2 padding;

	MaterialData data;
};

struct Offset
{
	uint material_idx;
	uint idx_offset;
	uint vertex_offset;
};

struct Ray
{
	float3 origin;
	float3 direction;
};

struct RayCone
{
	float width;
	float spread_angle;
};

#endif //__RT_STRUCTS__ 
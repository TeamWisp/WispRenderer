#ifndef __RT_SHADOW_ENTRIES__
#define __RT_SHADOW_ENTRIES__

#include "util.hlsl"
#include "pbr_util.hlsl"
#include "material_util.hlsl"

// Definitions for: 
// - Vertex, Material, Offset
// - Ray, RayCone, ReflectionHitInfo
#include "rt_structs.hlsl"

[shader("closesthit")]
void ShadowClosestHitEntry(inout ShadowHitInfo hit, Attributes bary)
{
	hit.is_hit = true;
}

[shader("miss")]
void ShadowMissEntry(inout ShadowHitInfo hit : SV_RayPayload)
{
	hit.is_hit = false;
}

//TODO: Add any hit shader

#endif //__RT_SHADOW_ENTRIES__
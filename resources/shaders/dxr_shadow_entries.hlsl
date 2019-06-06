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
	hit.is_hit = true;
}

[shader("miss")]
void ShadowMissEntry(inout ShadowHitInfo hit : SV_RayPayload)
{
	hit.is_hit = false;
}

#endif //__DXR_SHADOW_ENTRIES_HLSL__
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
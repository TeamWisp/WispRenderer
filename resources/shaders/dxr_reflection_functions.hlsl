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
#ifndef __DXR_REFLECTION_FUNCTIONS_HLSL__
#define __DXR_REFLECTION_FUNCTIONS_HLSL__

#include "dxr_global.hlsl"
#include "dxr_structs.hlsl"

// Definitions for: 
// - Vertex, Material, Offset
// - Ray, RayCone, ReflectionHitInfo
#include "dxr_structs.hlsl"

// Definitions for: 
// - HitWorldPosition, Load3x32BitIndices, unpack_position, HitAttribute
#include "dxr_functions.hlsl"

float3 TraceReflectionRay(float3 origin, float3 norm, float3 direction, uint rand_seed, uint depth, RayCone cone)
{

	if (depth >= MAX_RECURSION)
	{
		return skybox.SampleLevel(s0, direction, 0).rgb;
	}

	origin += norm * EPSILON;

	ReflectionHitInfo payload = {origin, float3(0,0,1), rand_seed, depth, cone};

	// Define a ray, consisting of origin, direction, and the min-max distance values
	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = direction;
	ray.TMin = 0;
	ray.TMax = 10000.0;

	// Trace the ray
	TraceRay(
		Scene,
		RAY_FLAG_NONE,
		//RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH
		//RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
		0xFF, // InstanceInclusionMask
		0, // RayContributionToHitGroupIndex
		1, // MultiplierForGeometryContributionToHitGroupIndex
		0, // miss shader index
		ray,
		payload);

	return payload.color;
}

float3 DoReflection(float3 wpos, float3 V, float3 normal, uint rand_seed, uint depth, RayCone cone)
{
	// Calculate ray info
	float3 reflected = normalize(reflect(-V, normal));

	// Shoot reflection ray
	return TraceReflectionRay(wpos, normal, reflected, rand_seed, depth, cone);
}
#endif //__DXR_REFLECTION_FUNCTIONS_HLSL__
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
#ifndef __DXR_SHADOWS_FUNCTIONS__
#define __DXR_SHADOWS_FUNCTIONS__

#include "dxr_global.hlsl"
#include "dxr_structs.hlsl"

bool TraceShadowRay(float3 origin, float3 direction, float far, uint calling_pass, unsigned int depth)
{
	if (depth >= MAX_RECURSION)
	{
		return false;
	}

	// Define a ray, consisting of origin, direction, and the min-max distance values
	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = direction;
	ray.TMin = 0;
	ray.TMax = far;

	ShadowHitInfo payload = { false, 0 };

	uint ray_contr_idx = 1;
	uint miss_idx = 1;

	if (calling_pass == CALLINGPASS_SHADOWS)
	{
		ray_contr_idx = 0;
		miss_idx = 0;
	}

	// Trace the ray
	TraceRay(
		Scene,
		RAY_FLAG_NONE,
		~0, // InstanceInclusionMask
		ray_contr_idx, // RayContributionToHitGroupIndex
		0, // MultiplierForGeometryContributionToHitGroupIndex
		miss_idx, // miss shader index is set to idx but can probably be anything.
		ray,
		payload);

	return payload.is_hit;
}

// Get shadow factor
float GetShadowFactor(float3 wpos, float3 light_dir, float light_size, float t_max, uint sample_count, uint depth, uint calling_pass, inout uint rand_seed)
{
	float shadow_factor = 0.0f;

#ifdef SOFT_SHADOWS
	for (uint i = 0; i < sample_count; ++i)
	{
		//float3 offset = normalize(float3(nextRand(rand_seed), nextRand(rand_seed), nextRand(rand_seed))) - 0.5;
		float3 dir = perturbDirectionVector(rand_seed, light_dir, light_size);
		float3 ray_direction = normalize(dir);

		bool shadow = TraceShadowRay(wpos, ray_direction, t_max, calling_pass, depth);

		shadow_factor += lerp(1.0, 0.0, shadow);
	}

	shadow_factor /= float(sample_count);

#else //SOFT_SHADOWS

	bool shadow = TraceShadowRay(wpos, light_dir, t_max, calling_pass, depth);

	shadow_factor = !shadow;

#endif //SOFT_SHADOWS

	return shadow_factor;
}

#endif //__DXR_SHADOWS_FUNCTIONS__
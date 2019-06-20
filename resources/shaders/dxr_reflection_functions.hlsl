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

#include "pbr_util.hlsl"

float3 TraceReflectionRay(float3 origin, float3 norm, float3 direction, uint rand_seed, uint depth, RayCone cone, inout float4 dir_t)
{

	if (depth >= MAX_RECURSION)
	{
		return skybox.SampleLevel(s0, direction, 0).rgb;
	}

	origin += norm * EPSILON;

	ReflectionHitInfo payload = {origin, float3(0,0,1), rand_seed, depth, 0, cone};

	// Define a ray, consisting of origin, direction, and the min-max distance values
	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = direction;
	ray.TMin = 0;
	ray.TMax = 10000.0;

	bool nan = isnan(origin.x) == true || isnan(origin.y) == true || isnan(origin.z) == true;
	nan = nan || isnan(direction.x) == true || isnan(direction.y) == true || isnan(direction.z) == true;
	if(nan)
	{
		return skybox.SampleLevel(s0, direction, 0).rgb;
	}

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

	dir_t = float4(direction, payload.hit_t);
	return payload.color;
}

float4 DoReflection(float3 wpos, float3 V, float3 N, uint rand_seed, uint depth, float roughness, float metallic, RayCone cone, inout float4 dir_t)
{
	// Calculate ray info
	float3 reflected = reflect(-V, N);

	// Shoot an importance sampled ray

	#ifndef PERFECT_MIRROR_REFLECTIONS


		// Shoot perfect mirror ray if enabled or if it's a recursion or it's almost a perfect mirror

		if (depth > 0 || roughness < 0.05f)
			return float4(TraceReflectionRay(wpos, N, reflected, rand_seed, depth, cone, dir_t), 1);

		#ifdef GROUND_TRUTH_REFLECTIONS

			float3 reflection = float3(0.f, 0.f, 0.f);
			float weight_sum = 0.f;
			float sampled_count = 0.f;
			float pdf_weight = 0.f;	/* Used for further weighting by spatial reconstruction */
			float3 total_hit = float3(0.f, 0.f, 0.f);

			//[unroll]
			for (uint i = 0; i < max(MAX_GT_REFLECTION_SAMPLES * metallic, 2); ++i) {

				nextRand(rand_seed);
				float2 xi = hammersley2d(rand_seed, 8192);
				float pdf = 0.f;
				float3 H = importanceSamplePdf(xi, roughness, N, pdf);
				float3 L = reflect(-V, H);

				float NdotL = max(dot(N, L), 0.f);

				if (NdotL <= 0.f) {
					nextRand(rand_seed);
					xi = hammersley2d(rand_seed, 8192);
					H = importanceSamplePdf(xi, roughness, N, pdf);
					L = reflect(-V, H);
					NdotL = max(dot(N, L), 0.f);
				}

				if (NdotL >= 0.f) {
					float weight = brdf_weight(V, L, N, roughness) / pdf;
					float4 ray_dir_t = float4(0.f, 0.f, 0.f, 0.f);
					reflection += TraceReflectionRay(wpos, N, L, rand_seed, depth, cone, ray_dir_t) * weight;
					weight_sum += weight;
					pdf_weight += pdf;
					total_hit += ray_dir_t.xyz * ray_dir_t.w;
					++sampled_count;
				}
			}
			total_hit /= sampled_count;
			dir_t = float4(normalize(total_hit), length(total_hit));
			return float4(reflection / weight_sum, pdf_weight / sampled_count);

		#else

			//Calculate an importance sampled ray

			nextRand(rand_seed);
			float2 xi = hammersley2d(rand_seed, 8192);
			float pdf = 0;
			float3 H = importanceSamplePdf(xi, roughness, N, pdf);
			float3 L = reflect(-V, H);

			float NdotL = max(dot(N, L), 0.f);

			if(NdotL <= 0.f)
			{
				nextRand(rand_seed);
				xi = hammersley2d(rand_seed, 8192);
				H = importanceSamplePdf(xi, roughness, N, pdf);
				L = reflect(-V, H);
				NdotL = max(dot(N, L), 0.f);
			}

			float3 reflection = float3(0.f, 0.f, 0.f);
			
			if (NdotL >= 0.f)
			{
				reflection = TraceReflectionRay(wpos, N, L, rand_seed, depth, cone, dir_t);
			}

			#ifndef DISABLE_SPATIAL_RECONSTRUCTION
			return float4(reflection, pdf);
			#else
			return float4(reflection, -1.f);
			#endif

		#endif

	#else
		// Shoot perfect mirror ray if enabled or if it's a recursion or it's almost a perfect mirror
		return float4(TraceReflectionRay(wpos, N, reflected, rand_seed, depth, cone, dir_t), -1.f);
	#endif
}
#endif //__DXR_REFLECTION_FUNCTIONS_HLSL__
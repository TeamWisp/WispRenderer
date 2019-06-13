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

float3 TraceReflectionRay(float3 origin, float3 norm, float3 direction, uint rand_seed, uint depth, RayCone cone, inout float4 dirT)
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

	dirT = float4(direction, payload.hitT);
	return payload.color;
}

float4 DoReflection(float3 wpos, float3 V, float3 N, uint rand_seed, uint depth, float roughness, float metallic, RayCone cone, inout float4 dirT)
{
	// Calculate ray info
	float3 reflected = reflect(-V, N);

	// Shoot an importance sampled ray

	#ifndef PERFECT_MIRROR_REFLECTIONS


		// Shoot perfect mirror ray if enabled or if it's a recursion or it's almost a perfect mirror

		if (depth > 0 || roughness < 0.05)
			return float4(TraceReflectionRay(wpos, N, reflected, rand_seed, depth, cone, dirT), 1);

		#ifdef GROUND_TRUTH_REFLECTIONS

			float3 reflection = float3(0, 0, 0);
			float weightSum = 0, sampled_count = 0;
			float pdfWeight = 0;	/* Used for further weighting by spatial reconstruction */
			float3 total_hit = float3(0, 0, 0);

			//[unroll]
			for (uint i = 0; i < max(MAX_GT_REFLECTION_SAMPLES * metallic, 2); ++i) {

				nextRand(rand_seed);
				float2 xi = hammersley2d(rand_seed, 8192);
				float pdf = 0;
				float3 H = importanceSamplePdf(xi, roughness, N, pdf);
				float3 L = reflect(-V, H);

				float NdotL = max(dot(N, L), 0);

				if (NdotL <= 0) {
					nextRand(rand_seed);
					xi = hammersley2d(rand_seed, 8192);
					H = importanceSamplePdf(xi, roughness, N, pdf);
					L = reflect(-V, H);
					NdotL = max(dot(N, L), 0);
				}

				if (NdotL >= 0) {
					float weight = brdf_weight(V, L, N, roughness) / pdf;
					float4 ray_dir_t;
					reflection += TraceReflectionRay(wpos, N, L, rand_seed, depth, cone, ray_dir_t) * weight;
					weightSum += weight;
					pdfWeight += pdf;
					total_hit += ray_dir_t.xyz * ray_dir_t.w;
					++sampled_count;
				}
			}

			dirT = float4(normalize(total_hit), length(total_hit) / sampled_count);
			return float4(reflection / weightSum, pdfWeight / sampled_count);

		#else

			//Calculate an importance sampled ray

			nextRand(rand_seed);
			float2 xi = hammersley2d(rand_seed, 8192);
			float pdf = 0;
			float3 H = importanceSamplePdf(xi, roughness, N, pdf);
			float3 L = reflect(-V, H);

			float NdotL = max(dot(N, L), 0);

			if(NdotL <= 0)
			{
				nextRand(rand_seed);
				xi = hammersley2d(rand_seed, 8192);
				H = importanceSamplePdf(xi, roughness, N, pdf);
				L = reflect(-V, H);
				NdotL = max(dot(N, L), 0);
			}

			float3 reflection = float3(0, 0, 0);
			
			if (NdotL >= 0)
			{
				reflection = TraceReflectionRay(wpos, N, L, rand_seed, depth, cone, dirT);
			}

			#ifndef DISABLE_SPATIAL_RECONSTRUCTION
			return float4(reflection, pdf);
			#else
			return float4(reflection, -1);
			#endif

		#endif

	#else
		// Shoot perfect mirror ray if enabled or if it's a recursion or it's almost a perfect mirror
		return float4(TraceReflectionRay(wpos, N, reflected, rand_seed, depth, cone, dirT), -1);
	#endif
}
#endif //__DXR_REFLECTION_FUNCTIONS_HLSL__
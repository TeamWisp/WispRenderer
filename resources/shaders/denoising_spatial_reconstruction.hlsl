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
 #ifndef __DENOISING_SPATIAL_RECONSTRUCTION_HLSL__
#define __DENOISING_SPATIAL_RECONSTRUCTION_HLSL__
#include "pbr_util.hlsl"
#include "rand_util.hlsl"

cbuffer CameraProperties : register(b0)
{
	float4x4 inv_vp;
	float4x4 inv_view;

	float padding;
	uint frame_idx;
	float near_plane, far_plane;
};

RWTexture2D<float4> filtered : register(u0);
Texture2D reflection_pdf : register(t0);
Texture2D dir_hit_t : register(t1);
Texture2D albedo_roughness : register(t2);
Texture2D normal_metallic : register(t3);
Texture2D depth_buffer : register(t4);
SamplerState nearest_sampler  : register(s0);
SamplerState linear_sampler : register(s1);

float linearize_depth(float D)
{
	float z_n = D * 2.0f - 1.0f;
	return 2.0f * near_plane * far_plane / (far_plane + near_plane - z_n * (far_plane - near_plane));
}

//Get total weight from neighbor using normal and normalized depth from both neighbor and center
float neighbor_edge_weight(float3 N, float3 N_neighbor, float D, float D_neighbor, float2 uv)
{
	return float(uv.x >= 0.f && uv.y >= 0.f && uv.x <= 1.f && uv.y <= 1.f) * (D != 1.0f && D_neighbor != 1.0f);
}

//Hardcode the samples for now; settings: kernelSize=5, points=16
//https://github.com/Nielsbishere/NoisePlayground/blob/master/bluenoise_test.py

static const uint sample_count = 16;
static const float2 samples[4][16] = {
		{
				float2(0.f ,  0.f), float2(-1.f ,  -1.f), float2(1.f ,  0.f), float2(-2.f ,  -1.f),
				float2(-2.f ,  1.f), float2(0.f ,  -3.f), float2(2.f ,  -2.f), float2(1.f ,  2.f),
				float2(0.f ,  3.f), float2(-4.f ,  -1.f), float2(0.f ,  -4.f), float2(-2.f ,  3.f),
				float2(-4.f ,  1.f), float2(3.f ,  1.f), float2(3.f ,  -3.f), float2(4.f ,  0.f)

		},
		{
				float2(0.f ,  0.f), float2(1.f ,  -1.f), float2(0.f ,  -2.f), float2(-1.f ,  -2.f),
				float2(-1.f ,  2.f), float2(0.f ,  2.f), float2(2.f ,  0.f), float2(-3.f ,  1.f),
				float2(-3.f ,  -2.f), float2(1.f ,  -3.f), float2(-3.f ,  2.f), float2(3.f ,  0.f),
				float2(-2.f ,  -4.f), float2(1.f ,  3.f), float2(-4.f ,  -2.f), float2(2.f ,  3.f)

		},
		{
				float2(0.f ,  0.f), float2(-1.f ,  0.f), float2(0.f ,  1.f), float2(-2.f ,  0.f),
				float2(2.f ,  -1.f), float2(-1.f ,  -3.f), float2(-3.f ,  -1.f), float2(2.f ,  2.f),
				float2(2.f ,  -3.f), float2(3.f ,  -1.f), float2(-3.f ,  -3.f), float2(-4.f ,  0.f),
				float2(-1.f ,  -4.f), float2(-4.f ,  2.f), float2(-3.f ,  3.f), float2(4.f ,  -1.f)

		},
		{
				float2(0.f ,  0.f), float2(0.f ,  -1.f), float2(-1.f ,  1.f), float2(-2.f ,  -2.f),
				float2(1.f ,  1.f), float2(1.f ,  -2.f), float2(-3.f ,  0.f), float2(2.f ,  1.f),
				float2(-2.f ,  -3.f), float2(-2.f ,  2.f), float2(-1.f ,  3.f), float2(1.f ,  -4.f),
				float2(3.f ,  -2.f), float2(3.f ,  2.f), float2(-4.f ,  -3.f), float2(0.f ,  4.f)

		}
};

//Sample a neighbor; 0,0 -> 1,1; outside of that range indicates an invalid uv
float2 sample_neighbor_uv(uint sampleId, uint2 full_res_pixel, uint2 resolution, float2 rand, float kernelSize)
{
	uint pixId = full_res_pixel.x % 2 + full_res_pixel.y % 2 * 2;
	float2 offset = samples[pixId][sampleId] * kernelSize;
	offset = mul(float2x2(rand.x, rand.y, -rand.y, rand.x), offset);
	return (float2(full_res_pixel / 2.f) + offset) / float2(resolution / 2.f - 1.f);
}

static const float3 luminance = float3(0.2126f, 0.7152f, 0.0722f);

float3 unpack_position(float2 uv, float depth)
{
  // Get world space position
  const float4 ndc = float4(uv * 2.0f - 1.0f, depth, 1.0f);
  float4 wpos = mul(inv_vp, ndc);
  return (wpos.xyz / wpos.w).xyz;
}

[numthreads(16, 16, 1)]
void main(int3 pix3 : SV_DispatchThreadID)
{
	//Get dispatch dimensions

	uint2 pix = uint2(pix3.xy);
	uint width, height;
	depth_buffer.GetDimensions(width, height);

	//Get per pixel values

	const float depth = depth_buffer[pix].r;
	const float2 uv = float2(pix.xy) / float2(width - 1.f, height - 1.f);
	const float3 pos = unpack_position(uv, depth);

	const float3 camera_pos = float3(inv_view[0][3], inv_view[1][3], inv_view[2][3]);
	const float3 V = normalize(camera_pos - pos);

	float roughness = albedo_roughness[pix].w;
	const float3 N = normalize(normal_metallic[pix].xyz);

	const float pdf = reflection_pdf.SampleLevel(nearest_sampler, uv, 0).w;

	float3 result3;

	//pdf < 0 disables spatial reconstruction
	if (pdf >= 0)
	{

		//Weigh the samples correctly

		float3 result = float3(0.f, 0.f, 0.f);
		float weight_sum = 0.0f;

		uint rand_seed = initRand(pix.x + pix.y * width, frame_idx);

		float distance = length(camera_pos - pos);
		float sampleCountScalar = lerp(1.f, (1.f - distance / far_plane) * roughness, 1);
		roughness = max(roughness, 1e-2);

		float kernel_size = 8.f;

		for (uint i = 0; i < max(ceil(kernel_size), 1); ++i) {

			//Get sample related data

			float2 random = float2(nextRand(rand_seed), nextRand(rand_seed));

			const float2 neighbor_uv = sample_neighbor_uv(i, pix, uint2(width, height), random, 1);

			const float depth_neighbor = depth_buffer.SampleLevel(nearest_sampler, neighbor_uv, 0).r;
			const float3 pos_neighbor = unpack_position(neighbor_uv, depth_neighbor);

			const float4 hit_t = dir_hit_t.SampleLevel(nearest_sampler, neighbor_uv, 0);

			const float3 V_neighbor = normalize(camera_pos - pos_neighbor);

			const float4 reflection_pdf_neighbor = reflection_pdf.SampleLevel(nearest_sampler, neighbor_uv, 0);
			if(reflection_pdf_neighbor.w>0.0)
			{
				const float3 color = clamp(reflection_pdf_neighbor.xyz, 0, 1);
				const float3 L = hit_t.xyz;
				const float pdf_neighbor = max(reflection_pdf_neighbor.w, 1e-5);
				const float3 N_neighbor = normalize(normal_metallic.SampleLevel(nearest_sampler, neighbor_uv, 0).xyz);
	
				//Calculate weight and weight sum
	
				const float neighbor_weight = neighbor_edge_weight(N, N_neighbor, depth, depth_neighbor, neighbor_uv);
				
				float weight = (brdf_weight(-V, L, N, roughness) / pdf_neighbor) * neighbor_weight;
				weight = isnan(weight) || isinf(weight) ? 0 : weight;
				result += color * weight;
				weight_sum += weight;
			}
		}

		result3 = result / weight_sum;
		if(weight_sum == 0.0f)
		{
			result3 = reflection_pdf.SampleLevel(nearest_sampler, uv, 0).xyz;
		}

		
		if(isnan(weight_sum) == true)
		{
			result3 = float3(0, 0, 1);
			filtered[pix] = float4(result3, 1);
			return;
		}
	}
	else
	{
		result3 = reflection_pdf.SampleLevel(nearest_sampler, uv, 0).xyz;
	}

	filtered[pix] = float4(result3, 1);

}

#endif
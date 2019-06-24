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
#ifndef __DENOISING_REFLECTIONS_HLSL__
#define __DENOISING_REFLECTIONS_HLSL__
#include "pbr_util.hlsl"
#include "rand_util.hlsl"

Texture2D input_texture : register(t0);
Texture2D ray_raw_texture : register(t1);
Texture2D dir_hit_t_texture : register(t2);
Texture2D albedo_roughness_texture : register(t3);
Texture2D normal_metallic_texture : register(t4);
Texture2D motion_texture : register(t5); // xy: motion, z: fwidth position, w: fwidth normal
Texture2D linear_depth_texture : register(t6);
Texture2D world_position_texture : register(t7);

Texture2D in_history_texture : register(t8);

Texture2D accum_texture : register(t9);
Texture2D prev_normal_texture : register(t10);
Texture2D prev_depth_texture : register(t11);
Texture2D in_moments_texture : register(t12);

RWTexture2D<float4> output_texture : register(u0);
RWTexture2D<float> out_history_texture : register(u1);
RWTexture2D<float2> out_moments_texture : register(u2);

SamplerState point_sampler : register(s0);
SamplerState linear_sampler : register(s0);

cbuffer CameraProperties : register(b0)
{
	float4x4 view;

	float4x4 projection;

	float4x4 inv_projection;

	float4x4 inv_view;

	float4x4 prev_projection;

	float4x4 prev_view;

	uint is_hybrid;
	uint is_path_tracer;
	uint is_ao;
	uint has_shadows;

	float3 padding1;
	uint has_reflections;
};

cbuffer DenoiserSettings : register(b1)
{
	float color_integration_alpha;
	float moments_integration_alpha;
	float variance_clipping_sigma;
	float roughness_reprojection_threshold;
	int max_history_samples;
	float n_phi;
	float z_phi;
	float l_phi;
};

cbuffer WaveletPass : register(b2)
{
	float wavelet_size;
}

float3 OctToDir(uint octo)
{
	float2 e = float2( f16tof32(octo & 0xFFFF), f16tof32((octo>>16) & 0xFFFF) ); 
	float3 v = float3(e, 1.0 - abs(e.x) - abs(e.y));
	if (v.z < 0.0f)
		v.xy = (1.0f - abs(v.yx)) * (step(0.0f, v.xy) * 2.0f - float2(1.0f,1.0f));
	return normalize(v);
}

float Luminance(float3 color)
{
	return (color.r + color.r + color.b + color.g + color.g + color.g) / 6.0f;
}

void FetchNormalAndLinearZ(in float2 ipos, out float3 norm, out float2 zLinear)
{
	norm = normal_metallic_texture[ipos].xyz;
	zLinear = linear_depth_texture[ipos].xy;
}

float NormalDistanceCos(float3 n1, float3 n2, float power)
{
	return pow(max(0.0f, dot(n1, n2)), power);
}

float ComputeWeight(
	float depth_center, float depth_p, float phi_depth,
	float3 normal_center, float3 normal_p, float norm_power, 
	float luminance_direct_center, float luminance_direct_p, float phi_direct)
{
	const float w_normal    = NormalDistanceCos(normal_center, normal_p, norm_power);
	const float w_z         = (phi_depth == 0.f) ? 0.0f : abs(depth_center - depth_p) / phi_depth;
	const float w_l_direct   = abs(luminance_direct_center - luminance_direct_p) / phi_direct;

	const float w_direct   = exp(0.0f - max(w_l_direct, 0.0f)   - max(w_z, 0.0f)) * w_normal;

	return w_direct;
}

float ComputeWeightNoLuminance(float depth_center, float depth_p, float phi_depth, float3 normal_center, float3 normal_p)
{
	const float w_normal    = NormalDistanceCos(normal_center, normal_p, 128.f);
	const float w_z         = abs(depth_center - depth_p) / phi_depth;

	return exp(-max(w_z, 0.0f)) * w_normal;
}

bool IsReprojectionValid(float2 coord, float z, float z_prev, float fwidth_z, float3 normal, float3 normal_prev, float fwidth_normal)
{
	int2 screen_size = int2(0, 0);
	output_texture.GetDimensions(screen_size.x, screen_size.y);

	bool ret = (coord.x >= 0.f && coord.x < screen_size.x && coord.y >= 0.f && coord.y < screen_size.y);

	ret = ret && ((distance(normal, normal_prev) / (fwidth_normal + 1e-2)) < 16.0);

	return ret;
}

bool LoadPrevData(float2 screen_coord, inout float2 found_pos, out float4 prev_direct, out float2 prev_moments, out float history_length)
{
	float2 screen_size = float2(0.f, 0.f);
	output_texture.GetDimensions(screen_size.x, screen_size.y);

	float2 uv = screen_coord / screen_size;

	float4 motion = motion_texture.SampleLevel(point_sampler, uv, 0);
	float2 q_uv = uv - motion.xy;

	float2 prev_coords = q_uv * screen_size;

	const float roughness = albedo_roughness_texture[screen_coord].w;

	float4 depth = linear_depth_texture[prev_coords];
	float3 normal = OctToDir(asuint(depth.w));
	prev_direct = float4(0.0f, 0.0f, 0.0f, 0.0f);
	prev_moments = float2(0.0f, 0.0f);
	bool v[4];
	const float2 pos_prev = prev_coords;
	int2 offset[4] = {int2(0, 0), int2(1, 0), int2(0, 1), int2(1, 1)};
	bool valid = false;
	[unroll]
	for(int sample_idx = 0; sample_idx < 4; ++sample_idx)
	{
		float2 loc = pos_prev + offset[sample_idx];
		float4 depth_prev = prev_depth_texture[loc];
		float3 normal_prev = OctToDir(asuint(depth_prev.w));
		v[sample_idx] = IsReprojectionValid(loc, depth.z, depth_prev.x, depth.y, normal, normal_prev, motion.w);
		valid = valid || v[sample_idx];
	}
	if(valid)
	{
		float sum_weights = 0.0f;
		float x = frac(pos_prev.x);
		float y = frac(pos_prev.y);
		float weights[4] = {(1.0f - x) * (1.0f - y),
							x * (1.0f - y),
							(1.0f - x) * y,
							x * y };
		[unroll]
		for(int sample_idx = 0; sample_idx < 4; ++sample_idx)
		{
			float2 loc = pos_prev + offset[sample_idx];
			prev_direct += weights[sample_idx] * accum_texture[loc] * float(v[sample_idx]);
			prev_moments += weights[sample_idx] * in_moments_texture[loc].xy * float(v[sample_idx]);
			sum_weights += weights[sample_idx] * float(v[sample_idx]);
		}
		valid = (sum_weights >= 0.01f);
		prev_direct = lerp(float4(0.0f, 0.0f, 0.0f, 0.0f), prev_direct / sum_weights, valid);
		prev_moments = lerp(float2(0.0f, 0.0f), prev_moments / sum_weights, valid);
	}
	if(!valid)
	{
		float cnt = 0.0f;
		const int radius = 1;
		for(int y = -radius; y <= radius; ++y)
		{
			for(int x = -radius; x <= radius; ++x)
			{
				float2 p = prev_coords + float2(x, y);
				float4 depth_filter = prev_depth_texture[p];
				float3 normal_filter = OctToDir(asuint(depth_filter.w));
				if(IsReprojectionValid(prev_coords, depth.z, depth_filter.x, depth.y, normal, normal_filter, motion.w))
				{
					prev_direct += accum_texture[p];
					prev_moments += in_moments_texture[p].xy;
					cnt += 1.0;
				}
			}
		}
		valid = cnt > 0;
		prev_direct /= lerp(1, cnt, cnt > 0);
		prev_moments /= lerp(1, cnt, cnt > 0);
	}
	if(valid)
	{
		history_length = in_history_texture[prev_coords].r;
	}
	else
	{
		prev_direct = float4(0.0f, 0.0f, 0.0f, 0.0f);
		prev_moments = float2(0.0f, 0.0f);
		history_length = 0;
	}
	found_pos = lerp(float2(-1.0f, -1.0f), pos_prev, valid);
	return valid;
}

float ComputeVarianceCenter(int2 center)
{
	float sum = 0.0f;

	const float kernel[2][2] = {
		{1.0f / 4.0f, 1.0f / 8.0f},
		{1.0f / 8.0f, 1.0f / 16.0f}
	};

	const int radius = 1;

	[unroll]
	for(int y = -radius; y <= radius; ++y)
	{
		[unroll]
		for(int x = -radius; x <= radius; ++x)
		{
			int2 p  = center + int2(x, y);

			float k = kernel[abs(x)][abs(y)];

			sum += input_texture[p].w * k;
		}
	}

	return sum;
}

float4 LineBoxIntersection(float3 box_min, float3 box_max, float3 c_in, float3 c_hist)
{
    float3 p_clip = 0.5f * (box_max + box_min);
    float3 e_clip = 0.5f * (box_max - box_min);

    float3 v_clip = c_hist - p_clip;
    float3 v_unit = v_clip.xyz / e_clip;
    float3 a_unit = abs(v_unit);
    float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));

    if(ma_unit > 1.0f)
    {
        return float4((p_clip + v_clip / ma_unit).xyz, ma_unit);
    }
    else
    {
        return float4(c_hist.xyz, ma_unit);
    }
}

[numthreads(16, 16, 1)]
void temporal_denoiser_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_coord = float2(dispatch_thread_id.xy);
	float2 screen_size = float2(0.0f, 0.0f);
	output_texture.GetDimensions(screen_size.x, screen_size.y);

	float2 uv = screen_coord / (screen_size - 1.0f);

	float4 accum_color = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float2 prev_moments = float2(0.0f, 0.0f);
	float history = 0;

	float2 prev_pos = float2(0.0f, 0.0f);

	bool valid = LoadPrevData(screen_coord, prev_pos, accum_color, prev_moments, history);
	history = lerp(0, history, valid);
	
	float4 input_color = input_texture[screen_coord];

	float pdf = ray_raw_texture.SampleLevel(point_sampler, uv, 0).w;

	if(pdf <= 0.0f)
	{
		output_texture[screen_coord] = float4(input_color.xyz, 0.f);
		return;
	}

	float3 moment_1 = float3(0.0f, 0.0f, 0.0f);
	float3 moment_2 = float3(0.0f, 0.0f, 0.0f);

	float3 clamp_min = 1.0f;
	float3 clamp_max = 0.0f;

	[unroll]
	for(int y = -3; y <= 3; ++y)
	{
		[unroll]
		for(int x = -3; x <= 3; ++x)
		{
			float3 color = input_texture[screen_coord + int2(x, y)].xyz;
			moment_1 += color;
			moment_2 += color * color;
			clamp_min = min(color, clamp_min);
			clamp_max = max(color, clamp_max);
		}
	}

	float3 mu = moment_1 / 49.0f;
	float3 sigma = sqrt(moment_2 / 49.0f - mu*mu);

	float3 box_min = mu - variance_clipping_sigma * sigma;
	float3 box_max = mu + variance_clipping_sigma * sigma;

	float4 clipped = LineBoxIntersection(box_min, box_max, input_color.xyz, accum_color.xyz);

	const float roughness = albedo_roughness_texture[screen_coord].w;

	//perhaps replace the 0.25 with a clamping strength variable for the settings screen
	accum_color = lerp(clipped, accum_color, pow(clamp(roughness, 0.0f, 1.0f), 0.25f));

	history = min(max_history_samples, history + 1);

	const float alpha = lerp(1.0f, max(color_integration_alpha, 1.0f/history), valid);
	const float moments_alpha = lerp(1.0f, max(moments_integration_alpha, 1.0f/history), valid);

	float2 cur_moments = float2(0.0f, 0.0f);
	cur_moments.x = Luminance(input_color.xyz);
	cur_moments.y = cur_moments.x * cur_moments.x;

	cur_moments = lerp(prev_moments, cur_moments, moments_alpha);

	float variance = cur_moments.y - cur_moments.x * cur_moments.x;

	float3 output = lerp(accum_color, input_color, alpha);
	
	output_texture[floor(screen_coord)] = float4(output.xyz, variance);
	out_history_texture[floor(screen_coord)] = history;
	out_moments_texture[floor(screen_coord)] = cur_moments;
}

[numthreads(16, 16, 1)]
void variance_estimator_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_coord = float2(dispatch_thread_id.xy);

	float2 screen_size = float2(0.f, 0.f);
	output_texture.GetDimensions(screen_size.x, screen_size.y);

	float2 uv = screen_coord / screen_size;

	float history = in_history_texture[screen_coord].r;
	[branch]
	if(history < 5.f)
	{		
		float sum_weights = 0.0f;
		float3 sum_direct = float3(0.0f, 0.0f, 0.0f);
		float2 sum_moments = float2(0.0f, 0.0f);

		const float4 direct_center = input_texture[screen_coord];

		float3 normal_center = float3(0.0f, 0.0f, 0.0f);
		float2 depth_center = float2(0.0f, 0.0f);

		FetchNormalAndLinearZ(screen_coord, normal_center, depth_center);

		if(depth_center.x < 0.0f)
		{
			output_texture[screen_coord] = direct_center;
			return;
		}
		
		const float phi_direct = l_phi;
		const float phi_depth = max(depth_center.y, 1e-8) * 3.0f;

		const int radius = 3.f;
		[unroll]
		for(int y = -radius; y <= radius; ++y)
		{
			[unroll]
			for(int x = -radius; x <= radius; ++x)
			{				
				const int2 p = screen_coord + int2(x, y);
				const bool inside = p.x >= 0.f && p.x < screen_size.x && p.y >= 0.f && p.y < screen_size.y;
				const bool same_pixel = (x==0.f) && (y==0.f);
				const float kernel = 1.0f;

				if(inside)
				{
					const float3 direct_p = input_texture[p].xyz;
					const float2 moments_p = in_moments_texture[p].xy;

					float3 normal_p;
					float2 z_p;
					FetchNormalAndLinearZ(p, normal_p, z_p);

					const float w = ComputeWeightNoLuminance(
						depth_center.x, z_p.x, phi_depth * length(float2(x, y)),
						normal_center, normal_p);

					if(isnan(direct_p.x) || isnan(direct_p.y) || isnan(direct_p.z) || isnan(w))
					{
						continue;
					}

					sum_weights += w;
					sum_direct += direct_p * w;

					sum_moments += moments_p * float2(w.xx);
				}
			}
		}

		sum_weights = max(sum_weights, 1e-6f);

		sum_direct /= sum_weights;
		sum_moments /= float2(sum_weights.xx);

		float variance = sum_moments.y - sum_moments.x * sum_moments.x;

		variance *= 5.0f/history;

		output_texture[screen_coord] = float4(sum_direct.xyz, variance);
	}
	else
	{
		output_texture[screen_coord] = input_texture[screen_coord];
	}
}

[numthreads(16, 16, 1)]
void spatial_denoiser_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
    float2 screen_coord = float2(dispatch_thread_id.xy);

    float2 screen_size = float2(0.0f, 0.0f);
    output_texture.GetDimensions(screen_size.x, screen_size.y);

	
    float2 uv = screen_coord / screen_size;
	
	const float eps_variance = 1e-10;
	const float kernel_weights[3] = {1.0f, 2.0f / 3.0f, 1.0f / 6.0f};

	const float4 direct_center = input_texture[screen_coord];
	const float luminance_direct_center = Luminance(direct_center.xyz);

	const float variance = ComputeVarianceCenter(int2(screen_coord.xy));

	if(isnan(variance))
	{
		output_texture[screen_coord] = float4(0.0f, 1.0f, 0.0f, 0.0f);
		return;
	}

	output_texture[screen_coord] = direct_center;
	
	const float history_length = in_history_texture[screen_coord].r;

	float3 normal_center;
	float2 depth_center;
	FetchNormalAndLinearZ(screen_coord, normal_center, depth_center);

	if(depth_center.x < 0.0f)
	{
		output_texture[screen_coord] = direct_center;
		return;
	}

	const float roughness = albedo_roughness_texture[screen_coord].w;

	const float phi_l_direct = l_phi * sqrt(max(0.0f, variance));
	const float phi_depth = max(depth_center.y, 1e-8) * wavelet_size;

	float sum_weights = 1.0f;
	float4 sum_direct = direct_center;

	[unroll]
	for(int y = -2; y <= 2; ++y)
	{
		[unroll]
		for(int x = -2; x <= 2; ++x)
		{			
			const int2 p = int2(screen_coord.xy) + int2(x, y) * wavelet_size;
			const bool inside = p.x >= 0 && p.x < screen_size.x && p.y >= 0 && p.y < screen_size.y;

			const float kernel = kernel_weights[abs(x)] * kernel_weights[abs(y)];

			if(inside && (x != 0 || y != 0))
			{				
				const float4 direct_p = input_texture[p];
				const float luminance_direct_p = Luminance(direct_p.xyz);

				float3 normal_p;
				float2 depth_p;
				FetchNormalAndLinearZ(p, normal_p, depth_p);		

				const float w = ComputeWeight(
					depth_center.x, depth_p.x, phi_depth*length(float2(x, y)),
					normal_center, normal_p, n_phi,
					luminance_direct_center, luminance_direct_p, phi_l_direct) * sqrt(max(1e-15f, roughness)); 

				const float w_direct = w * kernel;

				sum_weights += w_direct;
				sum_direct += float4(w_direct.xxx, w_direct * w_direct) * direct_p;
			}
		}
	}

	output_texture[screen_coord] = sum_direct / sum_weights;
}

#endif
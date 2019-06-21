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
#ifndef __DENOISING_SVGF_HLSL__
#define __DENOISING_SVGF_HLSL__

Texture2D input_texture : register(t0);
Texture2D motion_texture : register(t1);
Texture2D normal_texture : register(t2);
Texture2D depth_texture : register(t3);

Texture2D in_hist_length_texture : register(t4);

Texture2D prev_input_texture : register(t5);
Texture2D prev_moments_texture : register(t6);
Texture2D prev_normal_texture : register(t7);
Texture2D prev_depth_texture : register(t8);

RWTexture2D<float4> out_color_texture : register(u0);
RWTexture2D<float2> out_moments_texture : register(u1);
RWTexture2D<float> out_hist_length_texture : register(u2);

SamplerState point_sampler   : register(s0);
SamplerState linear_sampler  : register(s1);


cbuffer DenoiserSettings : register(b0)
{
	float blending_alpha;
	float blending_moments_alpha;
    float l_phi;
    float n_phi;
    float z_phi;
    float step_distance;
	float2 padding_2;
};

const static float VARIANCE_CLIPPING_GAMMA = 8.0;

float3 OctToDir(uint octo)
{
	float2 e = float2( f16tof32(octo & 0xFFFF), f16tof32((octo>>16) & 0xFFFF) ); 
	float3 v = float3(e, 1.0 - abs(e.x) - abs(e.y));
	if (v.z < 0.0)
		v.xy = (1.0 - abs(v.yx)) * (step(0.0, v.xy)*2.0 - (float2)(1.0));
	return normalize(v);
}

float Luminance(float3 color)
{
	return (color.r + color.r + color.b + color.g + color.g + color.g) / 6.0;
}

uint DirToOct(float3 normal)
{
	float2 p = normal.xy * (1.0 / dot(abs(normal), 1.0.xxx));
	float2 e = normal.z > 0.0 ? p : (1.0 - abs(p.yx)) * (step(0.0, p)*2.0 - (float2)(1.0));
	return (asuint(f32tof16(e.y)) << 16) + (asuint(f32tof16(e.x)));
}

void FetchNormalAndLinearZ(in int2 ipos, out float3 norm, out float2 zLinear)
{
	norm = normal_texture[ipos];
	zLinear = depth_texture[ipos].xy;
}

float NormalDistanceCos(float3 n1, float3 n2, float power)
{
	return pow(max(0.0, dot(n1, n2)), 128.0);
	//return pow( saturate(dot(n1,n2)), power);
	//return 1.0f;
}

float NormalDistanceTan(float3 a, float3 b)
{
	const float d = max(1e-8, dot(a, b));
	return sqrt(max(0.0, 1.0 - d * d)) / d;
}

float CalcWeights(
	float depth_center, float depth_p, float phi_depth,
	float3 normal_center, float3 normal_p, float norm_power, 
	float luminance_direct_center, float luminance_direct_p, float phi_direct)
{
	const float w_normal    = NormalDistanceCos(normal_center, normal_p, norm_power);
	const float w_z         = (phi_depth == 0) ? 0.0f : abs(depth_center - depth_p) / phi_depth;
	const float w_l_direct   = abs(luminance_direct_center - luminance_direct_p) / phi_direct;

	const float w_direct   = exp(0.0 - max(w_l_direct, 0.0)   - max(w_z, 0.0)) * w_normal;

	return w_direct;
}

float ComputeWeightNoLuminance(float depth_center, float depth_p, float phi_depth, float3 normal_center, float3 normal_p)
{
	const float w_normal    = NormalDistanceCos(normal_center, normal_p, n_phi);
	const float w_z         = abs(depth_center - depth_p) / phi_depth;

	return exp(-max(w_z, 0.0)) * w_normal;
}

bool IsReprojectionValid(int2 coord, float z, float z_prev, float fwidth_z, float3 normal, float3 normal_prev, float fwidth_normal)
{
	int2 screen_size = int2(0, 0);
	input_texture.GetDimensions(screen_size.x, screen_size.y);

	bool ret = (coord.x > -1 && coord.x < screen_size.x && coord.y > -1 && coord.y < screen_size.y);

	ret = ret && ((abs(z_prev - z) / (fwidth_z + 1e-4)) < 2.0);

	ret = ret && ((distance(normal, normal_prev) / (fwidth_normal + 1e-2)) < 16.0);

	return ret;
}

bool LoadPrevData(float2 screen_coord, out float4 prev_direct, out float2 prev_moments, out float history_length)
{
	float2 screen_size = float2(0.f, 0.f);
	input_texture.GetDimensions(screen_size.x, screen_size.y);

	float2 uv = screen_coord / screen_size;

	float4 motion = motion_texture.SampleLevel(point_sampler, uv, 0);
	float2 q_uv = uv - motion.xy;

	float2 prev_coords = q_uv * screen_size;

	float4 depth = depth_texture[prev_coords];
	float3 normal = OctToDir(asuint(depth.w));

	prev_direct = float4(0, 0, 0, 0);
	prev_moments = float2(0, 0);

	bool v[4];
	const float2 pos_prev = prev_coords;
	int2 offset[4] = {int2(0, 0), int2(1, 0), int2(0, 1), int2(1, 1)};

	bool valid = false;
	[unroll]
	for(int sample_idx = 0; sample_idx < 4; ++sample_idx)
	{
		int2 loc = int2(pos_prev) + offset[sample_idx];
		float4 depth_prev = prev_depth_texture[loc];
		float3 normal_prev = OctToDir(asuint(depth_prev.w));

		v[sample_idx] = IsReprojectionValid(loc, depth.z, depth_prev.x, depth.y, normal, normal_prev, motion.w);

		valid = valid || v[sample_idx];
	}

	if(valid)
	{
		float sum_weights = 0;
		float x = frac(pos_prev.x);
		float y = frac(pos_prev.y);

		float weights[4] = {(1 - x) * (1 - y),
							x * (1 - y),
							(1 - x) * y,
							x * y };

		[unroll]
		for(int sample_idx = 0; sample_idx < 4; ++sample_idx)
		{
			int2 loc = int2(pos_prev) + offset[sample_idx];

			prev_direct += weights[sample_idx] * prev_input_texture[loc] * float(v[sample_idx]);
			prev_moments += weights[sample_idx] * prev_moments_texture[loc] * float(v[sample_idx]);
			sum_weights += weights[sample_idx] * float(v[sample_idx]);
		}

		valid = (sum_weights >= 0.01);
		prev_direct = lerp(float4(0, 0, 0, 0), prev_direct / sum_weights, valid);
		prev_moments = lerp(float2(0, 0), prev_moments / sum_weights, valid);

	}
	if(!valid)
	{
		float cnt = 0.0;

		const int radius = 1;
		for(int y = -radius; y <= radius; ++y)
		{
			for(int x = -radius; x <= radius; ++x)
			{
				int2 p = prev_coords + int2(x, y);
				float4 depth_filter = prev_depth_texture[p];
				float3 normal_filter = OctToDir(asuint(depth_filter.w));

				if(IsReprojectionValid(prev_coords, depth.z, depth_filter.x, depth.y, normal, normal_filter, motion.w))
				{
					prev_direct += prev_input_texture[p];
					prev_moments += prev_moments_texture[p];
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
		history_length = in_hist_length_texture[prev_coords].r;
	}
	else
	{
		prev_direct = float4(0, 0, 0, 0);
		prev_moments = float2(0, 0);
		history_length = 0;
	}

	return valid;
}

float ComputeVarianceCenter(int2 center)
{
	float sum = 0.0;

	const float kernel[2][2] = {
		{1.0 / 4.0, 1.0 / 8.0},
		{1.0 / 8.0, 1.0 / 16.0}
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
    float3 p_clip = 0.5 * (box_max + box_min);
    float3 e_clip = 0.5 * (box_max - box_min);

    float3 v_clip = c_hist - p_clip;
    float3 v_unit = v_clip.xyz / e_clip;
    float3 a_unit = abs(v_unit);
    float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));

    if(ma_unit > 1.0)
    {
        return float4((p_clip + v_clip / ma_unit).xyz, ma_unit);
    }
    else
    {
        return float4(c_hist.xyz, ma_unit);
    }
}

[numthreads(16,16,1)]
void reprojection_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	out_color_texture.GetDimensions(screen_size.x, screen_size.y);

	float2 uv = float2(dispatch_thread_id.x / screen_size.x, dispatch_thread_id.y / screen_size.y);

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y);

	float4 direct = input_texture[screen_coord];

	float4 prev_direct = float4(0.0, 0.0, 0.0, 0.0);
	float2 prev_moments = float2(0.0, 0.0);
	float history_length = 0.0;

	bool success = LoadPrevData(screen_coord, prev_direct, prev_moments, history_length);

	float3 moment_1 = float3(0.0, 0.0, 0.0);
	float3 moment_2 = float3(0.0, 0.0, 0.0);

	float3 clamp_min = 1.0;
	float3 clamp_max = 0.0;

	[unroll]
	for(int y = -2; y <= 2; ++y)
	{
		[unroll]
		for(int x = -2; x <= 2; ++x)
		{
			float3 color = input_texture[screen_coord + int2(x, y)].xyz;
			moment_1 += color;
			moment_2 += color * color;
			clamp_min = min(color, clamp_min);
			clamp_max = max(color, clamp_max);
		}
	}

	float3 mu = moment_1 / 25.0;
	float3 sigma = sqrt(moment_2 / 25.0 - mu*mu);

	float3 box_min = max(mu - VARIANCE_CLIPPING_GAMMA * sigma, clamp_min);
	float3 box_max = min(mu + VARIANCE_CLIPPING_GAMMA * sigma, clamp_max);

	float4 clipped = LineBoxIntersection(box_min, box_max, direct.xyz, prev_direct.xyz);

	//success = clipped.w > 1.0;

	prev_direct = float4(clipped.xyz, prev_direct.w);

	history_length = min(32.0, success ? (history_length + 1.0) : 1.0);

	const float alpha = lerp(1.0, max(blending_alpha, 1.0/history_length), success);
	const float moments_alpha = lerp(1.0, max(blending_moments_alpha, 1.0/history_length), success);

	float2 moments = float2(0, 0);
	moments.r = Luminance(direct.xyz);
	moments.g = moments.r * moments.r;

	moments = lerp(prev_moments, moments, moments_alpha);

	out_moments_texture[screen_coord] = moments;
	out_hist_length_texture[screen_coord] = history_length;

	float variance = max(0.f, moments.g - moments.r * moments.r);

	direct = lerp(prev_direct, direct, alpha);

	out_color_texture[screen_coord] = float4(direct.xyz, variance);
}

[numthreads(16,16,1)]
void filter_moments_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	out_color_texture.GetDimensions(screen_size.x, screen_size.y);

	float2 uv = float2(dispatch_thread_id.x / screen_size.x, dispatch_thread_id.y / screen_size.y);

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y);

	float h = in_hist_length_texture[screen_coord].r;

	if(h < 4.0)
	{
		float sum_weights = 0.0;
		float3 sum_direct = float3(0.0, 0.0, 0.0);
		float2 sum_moments = float2(0.0, 0.0);

		const float4 direct_center = input_texture[screen_coord];
		const float luminance_direct_center = Luminance(direct_center.xyz);

		float3 normal_center = float3(0.0, 0.0, 0.0);
		float2 depth_center = float2(0.0, 0.0);

		FetchNormalAndLinearZ(screen_coord, normal_center, depth_center);


		if(depth_center.x < 0.0)
		{
			out_color_texture[screen_coord] = direct_center;
			return;
		}

		const float phi_direct = l_phi;
		const float phi_depth = max(depth_center.y, 1e-8) * 3.0;

		const int radius = 3;
		[unroll]
		for(int y = -radius; y <= radius; ++y)
		{
			[unroll]
			for(int x = -radius; x <= radius; ++x)
			{
				const int2 p = screen_coord + int2(x, y);
				const bool inside = p.x >= 0 && p.x < screen_size.x && p.y >= 0 && p.y < screen_size.y;
				const bool same_pixel = (x==0) && (y==0);
				const float kernel = 1.0;

				if(inside)
				{
					const float3 direct_p = input_texture[p].xyz;
					const float2 moments_p = prev_moments_texture[p].xy;

					const float l_direct_p = Luminance(direct_p);

					float3 normal_p;
					float2 z_p;
					FetchNormalAndLinearZ(p, normal_p, z_p);

					const float w = CalcWeights(
						depth_center.x, z_p.x, phi_depth * length(float2(x, y)),
						normal_center, normal_p, n_phi,
						luminance_direct_center, l_direct_p, l_phi);

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

		variance *= 4.0/h;

		out_color_texture[screen_coord] = float4(sum_direct.xyz, variance);
	}
	else
	{
		out_color_texture[screen_coord] = input_texture[screen_coord];
	}
}

[numthreads(16,16,1)]
void wavelet_filter_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	out_color_texture.GetDimensions(screen_size.x, screen_size.y);

	float2 uv = float2(dispatch_thread_id.x / screen_size.x, dispatch_thread_id.y / screen_size.y);

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y);

	const float eps_variance = 1e-10;
	const float kernel_weights[3] = {1.0, 2.0 / 3.0, 1.0 / 6.0};

	const float4 direct_center = input_texture[screen_coord];

	const float luminance_direct_center = Luminance(direct_center.xyz);

	const float variance = ComputeVarianceCenter(int2(screen_coord.xy));

	out_color_texture[screen_coord] = direct_center;
	
	const float history_length = in_hist_length_texture[screen_coord].r;

	float3 normal_center;
	float2 depth_center;
	FetchNormalAndLinearZ(screen_coord, normal_center, depth_center);

	if(depth_center.x < 0)
	{
		out_color_texture[screen_coord] = direct_center;
		return;
	}

	const float phi_l_direct = l_phi * sqrt(max(0.0, eps_variance + variance));
	const float phi_depth = max(depth_center.y, 1e-8) * step_distance;

	float sum_weights = 1.0;
	float4 sum_direct = direct_center;

	[unroll]
	for(int y = -2; y <= 2; ++y)
	{
		[unroll]
		for(int x = -2; x <= 2; ++x)
		{
			const int2 p = int2(screen_coord.xy) + int2(x, y) * step_distance;
			const bool inside = p.x >= 0 && p.x < screen_size.x && p.y >= 0 && p.y < screen_size.y;

			const float kernel = kernel_weights[abs(x)] * kernel_weights[abs(y)];

			if(inside && (x != 0 || y != 0))
			{
				const float4 direct_p = input_texture[p];

				float3 normal_p;
				float2 depth_p;
				FetchNormalAndLinearZ(p, normal_p, depth_p);

				const float luminance_direct_p = Luminance(direct_p.xyz);

				const float w = CalcWeights(
					depth_center.x, depth_p.x, phi_depth*length(float2(x, y)),
					normal_center, normal_p, n_phi,
					luminance_direct_center, luminance_direct_p, phi_l_direct
				);

				const float w_direct = w * kernel;

				sum_weights += w_direct;
				sum_direct += float4(w_direct.xxx, w_direct * w_direct) * direct_p;
			}
		}
	}

	out_color_texture[screen_coord] = float4(sum_direct / float4(sum_weights.xxx, sum_weights * sum_weights));
}

#endif //__DENOISING_SVGF_HLSL__
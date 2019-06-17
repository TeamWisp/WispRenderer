#ifndef __DENOISING_REFLECTIONS_HLSL__
#define __DENOISING_REFLECTIONS_HLSL__
#include "pbr_util.hlsl"
#include "rand_util.hlsl"

Texture2D input_texture : register(t0);
Texture2D ray_raw_texture : register(t1);
Texture2D dir_hitT_texture : register(t2);
Texture2D albedo_roughness_texture : register(t3);
Texture2D normal_metallic_texture : register(t4);
Texture2D motion_texture : register(t5); // xy: motion, z: fwidth position, w: fwidth normal
Texture2D linear_depth_texture : register(t6);
Texture2D world_position_texture : register(t7);

Texture2D in_history_texture : register(t8);

Texture2D accum_texture : register(t9);
Texture2D prev_normal_texture : register(t10);
Texture2D prev_depth_texture : register(t11);
Texture2D prev_position_texture : register(t12);
Texture2D prev_dir_hitT_texture : register(t13);
Texture2D in_moments_texture : register(t14);

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
	uint has_reflections;
	float3 padding1;
};

cbuffer DenoiserSettings : register(b1)
{
	float integration_alpha;
	float variance_clipping_sigma;
	float roughness_reprojection_threshold;
	int max_history_samples;
	float2 padding2;
	float n_phi;
	float z_phi;
};

float3 unpack_position(float2 uv, float depth, float4x4 proj_inv, float4x4 view_inv) {
	const float4 ndc = float4(uv * 2.0f - 1.f, depth, 1.0f);
	const float4 pos = mul( view_inv, mul(proj_inv, ndc));
	return (pos / pos.w).xyz;
}

float3 unpack_position_linear_depth(float2 uv, float depth, float4x4 proj_inv, float4x4 view_inv) {
	const float4 ndc = float4(uv * 2.0f - 1.f, 1.f, 1.0f);
	const float4 pos = mul( view_inv, mul(proj_inv, ndc));
	return normalize((pos / pos.w).xyz) * depth;
}

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
	norm = normal_metallic_texture[ipos].xyz;
	zLinear = linear_depth_texture[ipos].xy;
}

float NormalDistanceCos(float3 n1, float3 n2, float power)
{
	return pow(max(0.0, dot(n1, n2)), power);
}

float ComputeWeightNoLuminance(float depth_center, float depth_p, float phi_depth, float3 normal_center, float3 normal_p)
{
	const float w_normal    = NormalDistanceCos(normal_center, normal_p, 128.f);
	const float w_z         = abs(depth_center - depth_p) / phi_depth;

	return exp(-max(w_z, 0.0)) * w_normal;
}

float ComputeWeight(
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

bool IsReprojectionValid(int2 coord, float z, float z_prev, float fwidth_z, float3 normal, float3 normal_prev, float fwidth_normal)
{
	int2 screen_size = int2(0, 0);
	output_texture.GetDimensions(screen_size.x, screen_size.y);

	bool ret = (coord.x > -1 && coord.x < screen_size.x && coord.y > -1 && coord.y < screen_size.y);

	ret = ret && ((abs(z_prev - z) / (fwidth_z + 1e-4)) < 4.0);

	ret = ret && ((distance(normal, normal_prev) / (fwidth_normal + 1e-2)) < 16.0);

	return ret;
}

float3 GetReflectionWorldPos(float2 screen_coord, Texture2D origin_pos_texture, Texture2D reflection_dir_texture)
{
	float2 screen_size = float2(0, 0);
	output_texture.GetDimensions(screen_size.x, screen_size.y);
	float4 ray_data = reflection_dir_texture.SampleLevel(point_sampler, screen_coord / screen_size, 0);
	return origin_pos_texture.SampleLevel(point_sampler, screen_coord / screen_size, 0).xyz + ray_data.xyz * ray_data.w;
}

bool LoadPrevData(float2 screen_coord, inout float2 found_pos, out float4 prev_direct, out float history_length)
{
	float2 screen_size = float2(0.f, 0.f);
	output_texture.GetDimensions(screen_size.x, screen_size.y);

	float2 uv = screen_coord / screen_size;

	float4 motion = motion_texture.SampleLevel(point_sampler, uv, 0);
	float2 q_uv = uv - motion.xy;

	float2 prev_coords = q_uv * screen_size;

	const float roughness = albedo_roughness_texture[screen_coord].w;

	if(roughness < roughness_reprojection_threshold && dir_hitT_texture.SampleLevel(point_sampler, uv, 0).w == 0.0)
	{
		int step_size = 4;
		float2 best_pos = screen_coord;
		float2 center_pos = screen_coord;
		float3 coord_world_pos = GetReflectionWorldPos(screen_coord, world_position_texture, dir_hitT_texture);
		float best_dist = length(coord_world_pos - GetReflectionWorldPos(screen_coord, prev_position_texture, prev_dir_hitT_texture));

		if(prev_coords.x >= 0 && prev_coords.x < screen_size.x && prev_coords.y >= 0 && prev_coords.y < screen_size.y)
		{
			float motion_vec_dist = length(GetReflectionWorldPos(screen_coord, world_position_texture, dir_hitT_texture) -
				GetReflectionWorldPos(prev_coords, prev_position_texture, prev_dir_hitT_texture));
			if(motion_vec_dist < best_dist)
			{
				best_dist = motion_vec_dist;
				best_pos = prev_coords;
				center_pos = best_pos;
			}
		}

		float2 LDSP[4] = 
		{
			{1, 0},
			{0, -1},
			{-1, 0},
			{0, 1}
		};

		while(true)
		{
			[unroll]
			for(int i = 0; i < 4; ++i)
			{
				float2 position = center_pos + LDSP[i] * step_size;
				if(position.x >= 0 && position.x < screen_size.x && position.y >= 0 && position.y < screen_size.y)
				{
					float3 prev_pos = GetReflectionWorldPos(position, prev_position_texture, prev_dir_hitT_texture);
					if(length(coord_world_pos - prev_pos) < best_dist)
					{
						best_dist = length(coord_world_pos - prev_pos);
						best_pos = position;
					}
				}
			}

			if(best_pos.x == center_pos.x && best_pos.y == center_pos.y)
			{
				if(step_size==1)
				{
					break;
				}
				else
				{
					step_size /= 2;
				}
			}
			center_pos = best_pos;
		}

		float2 SDSP[8] = 
		{
			{1, 0},
			{1, -1},
			{0, -1},
			{-1, -1},
			{-1, 0},
			{-1, 1},
			{0, 1},
			{1, 1}
		};

		[unroll]
		for(int i = 0; i < 8; ++i)
		{
			float2 position = center_pos + SDSP[i];
			if(position.x >= 0 && position.x < screen_size.x && position.y >= 0 && position.y < screen_size.y)
			{
				float3 prev_pos = GetReflectionWorldPos(position, prev_position_texture, prev_dir_hitT_texture);
				if(length(coord_world_pos - prev_pos) < best_dist)
				{
					best_dist = length(coord_world_pos - prev_pos);
					best_pos = position;
				}
			}
		}

		prev_direct = accum_texture[best_pos];
		history_length = in_history_texture[best_pos].r;
		found_pos = screen_coord - best_pos;
		
		return best_dist < 100.0;
	}
	else
	{
		
		float4 depth = linear_depth_texture[prev_coords];
		float3 normal = OctToDir(asuint(depth.w));

		prev_direct = float4(0, 0, 0, 0);

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
				prev_direct += weights[sample_idx] * accum_texture[loc] * float(v[sample_idx]);
				sum_weights += weights[sample_idx] * float(v[sample_idx]);
			}
			valid = (sum_weights >= 0.01);
			prev_direct = lerp(float4(0, 0, 0, 0), prev_direct / sum_weights, valid);
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
						prev_direct += accum_texture[p];
						cnt += 1.0;
					}
				}
			}
			valid = cnt > 0;
			prev_direct /= lerp(1, cnt, cnt > 0);
		}
		if(valid)
		{
			history_length = in_history_texture[prev_coords].r;
		}
		else
		{
			prev_direct = float4(0, 0, 0, 0);
			history_length = 0;
		}

		found_pos = lerp(float2(-1, -1), pos_prev, valid);

		return valid;
	}
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

[numthreads(16, 16, 1)]
void temporal_denoiser_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_coord = float2(dispatch_thread_id.xy);
	float2 screen_size = float2(0, 0);
	output_texture.GetDimensions(screen_size.x, screen_size.y);

	float2 uv = screen_coord / screen_size;

	float4 accum_color = float4(0, 0, 0, 0);
	float history = 0;

	float2 prev_pos = float2(0, 0);

	float2 screen_center = screen_size / 4;

	bool valid = LoadPrevData(screen_coord, prev_pos, accum_color, history);
	history = lerp(0, history, valid);

	float4 input_color = input_texture[screen_coord];

	float pdf = ray_raw_texture.SampleLevel(point_sampler, uv, 0).w;

	if(pdf <= 0.0)
	{
		output_texture[screen_coord] = input_color;
		return;
	}

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

	float3 box_min = max(mu - variance_clipping_sigma * sigma, clamp_min);
	float3 box_max = min(mu + variance_clipping_sigma * sigma, clamp_max);

	float4 clipped = LineBoxIntersection(box_min, box_max, input_color.xyz, accum_color.xyz);

	accum_color = clipped;

	history = min(max_history_samples, history + 1);

	const float alpha = lerp(1.0, max(integration_alpha, 1.0/history), valid);

	float3 output = lerp(accum_color, input_color, alpha);
	
	output_texture[screen_coord] = float4(output.xyz, history);
	out_history_texture[screen_coord] = history;
}

[numthreads(16, 16, 1)]
void spatial_denoiser_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
    float2 screen_coord = float2(dispatch_thread_id.xy);

    float2 screen_size = float2(0, 0);
    output_texture.GetDimensions(screen_size.x, screen_size.y);

	
    float2 uv = screen_coord / screen_size;

    float3 world_pos = world_position_texture[screen_coord].xyz;
    float3 cam_pos = float3(inv_view[0][3], inv_view[1][3], inv_view[2][3]);

	float3 p_normal = float3(0, 0, 0);
	float2 p_depth = float2(0, 0);

	FetchNormalAndLinearZ(screen_coord, p_normal, p_depth);

    float3 V = normalize(cam_pos - world_pos);
	
	float pdf = ray_raw_texture.SampleLevel(point_sampler, uv, 0).w;
	if(pdf<0.0)
	{
		output_texture[screen_coord] = input_texture[screen_coord];
		return;
	}

	float3 center_normal = float3(0, 0, 0);
	float2 center_depth = float2(0, 0);

	FetchNormalAndLinearZ(screen_coord, center_normal, center_depth);

	float3 N = center_normal.xyz;
	float3 L = reflect(-V, N);
	float roughness = max(albedo_roughness_texture[screen_coord].a, 1e-2);

	float weights = 1.0;
	float4 accum = input_texture[screen_coord];

	const int kernel_size = 1 + 31 * roughness;

	const float kernel_weights[3] = {1.0, 2.0/3.0, 1.0/6.0};
	for(int x = -floor(kernel_size/2); x <= floor(kernel_size/2); ++x)
	{
		for(int y = -floor(kernel_size/2); y <= floor(kernel_size/2); ++y)
		{
			float2 location = screen_coord + float2(x, y);

			float pdf = ray_raw_texture.SampleLevel(point_sampler, location / screen_size, 0).w;

			if(pdf <= 0 || location.x < 0 || location.x >= screen_size.x || location.y < 0 || location.y >= screen_size.y)
			{
				
			}
			else
			{
				float4 color = input_texture[location];

				bool4 nan = isnan(color);
				if(nan.x == true || nan.y == true || nan.z == true || nan.w == true)
				{
					output_texture[screen_coord] = float4(1, 0, 0, 1);
					return;
				}

				float3 world_pos = world_position_texture[location].xyz;
				float3 V = normalize(cam_pos - world_pos);

				float3 p_normal = float3(0.0, 0.0, 1.0);
				float2 p_depth = float2(0.0, 0.0);

				FetchNormalAndLinearZ(location, p_normal, p_depth);

				float phi_depth = max(center_depth.y, 1e-8) * length(location - screen_coord);

				float weight = brdf_weight(V, L, N, roughness) * 
				max(ComputeWeightNoLuminance(center_depth.x, p_depth.x, phi_depth, center_normal, p_normal), 1e-5);

				weight *= x!=0 || y!=0;

				weights += weight;
				accum += color * weight;
			}
		}
	}

	output_texture[screen_coord] = accum / weights;
}

#endif
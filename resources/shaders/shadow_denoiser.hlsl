//Ignore this file. This is just here as reference material.

Texture2D input_texture : register(t0);
Texture2D depth_texture : register(t1);
Texture2D normal_texture : register(t2);
Texture2D velocity_texture : register(t3);
Texture2D kernel_texture : register(t4);
Texture2D accum_texture : register(t5);
Texture2D variance_in_texture : register(t6);
RWTexture2D<float4> output_texture   : register(u0);
RWTexture2D<float4> variance_out_texture : register(u1);
SamplerState point_sampler   : register(s0);
SamplerState linear_sampler  : register(s1);

cbuffer CameraProperties : register(b0)
{
	float4x4 view;
    float4x4 prev_view;
	float4x4 inv_view;
	float4x4 projection;
    float4x4 prev_projection;
	float4x4 inv_projection;
};

cbuffer DenoiserSettings : register(b1)
{
    float2 direction;
    int2 kernel_size;
    float depth_contrast;
    float c_phi;
    float p_phi;
    float n_phi;
    float step_distance;
};

const static float2 VARIANCE_CLIPPING_KERNEL = float2(7, 7);
const static float VARIANCE_CLIPPING_GAMMA = 0.75;

const static float WEIGHTS[5] = {0.0625, 0.25, 0.375, 0.25, 0.0625};

float3 unpack_position(float2 uv, float depth, float4x4 proj_inv, float4x4 view_inv) {
	const float4 ndc = float4(uv * 2.0 - 1.0, depth, 1.0);
	const float4 pos = mul( view_inv, mul(proj_inv, ndc));
	return (pos / pos.w).xyz;
}

float4 line_box_intersection(float3 box_min, float3 box_max, float4 c_in, float4 c_hist)
{
    float3 p_clip = 0.5 * (box_max + box_min);
    float3 e_clip = 0.5 * (box_max - box_min);

    float4 v_clip = c_hist - float4(p_clip, c_in.w);
    float3 v_unit = v_clip.xyz / e_clip;
    float3 a_unit = abs(v_unit);
    float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));

    if(ma_unit > 1.0)
    {
        return float4(p_clip, c_in.w) + v_clip / ma_unit;
    }
    else
    {
        return c_hist;
    }
}

[numthreads(16,16,1)]
void temporal_accumulator_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	output_texture.GetDimensions(screen_size.x, screen_size.y);

	float2 uv = float2(dispatch_thread_id.x / screen_size.x, dispatch_thread_id.y / screen_size.y);

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y);

    float2 q_UV = uv - velocity_texture.SampleLevel(point_sampler, uv, 0);

    int2 accum_coord = int2(q_UV.x * screen_size.x, q_UV.y*screen_size.y);

	float4 accum_data = accum_texture[accum_coord];
	float4 noise = input_texture[screen_coord];

    float3 m1 = float3(0,0,0);
    float3 m2 = float3(0,0,0);

    float2 center = float2(floor(VARIANCE_CLIPPING_KERNEL.x / 2.0), floor(VARIANCE_CLIPPING_KERNEL.y / 2.0));

    float3 clamp_min = 1.0;
    float3 clamp_max = 0.0;

    for(int i = 0; i < VARIANCE_CLIPPING_KERNEL.x; ++i)
    {
        for(int j = 0; j < VARIANCE_CLIPPING_KERNEL.y; ++j)
        {
            float3 noise_sample = input_texture[int2(screen_coord + int2(i, j) - center)];
            m1 += noise_sample;
            m2 += noise_sample*noise_sample;
            clamp_min = min(noise_sample, clamp_min);
            clamp_max = max(noise_sample, clamp_max);
        }
    }

    float3 mu = m1 / (VARIANCE_CLIPPING_KERNEL.x * VARIANCE_CLIPPING_KERNEL.y);
    float3 sigma = sqrt(m2 / (VARIANCE_CLIPPING_KERNEL.x * VARIANCE_CLIPPING_KERNEL.y) - mu * mu);

    float3 box_min = max(mu - VARIANCE_CLIPPING_GAMMA * sigma, clamp_min);
    float3 box_max = min(mu + VARIANCE_CLIPPING_GAMMA * sigma, clamp_max);

	float mean = accum_data.x / accum_data.w;

    float variance = sigma;

	if(accum_data.w==0.f)
	{
		accum_data = float4(1, 1, 1, 0);
	}

	accum_data.w = clamp(accum_data.w, 1.0, 128.0);

    accum_data.xyz = line_box_intersection(box_min, box_max, noise, accum_data);

    accum_data = lerp(accum_data, float4(1,1,1,0), q_UV.x < 0 || q_UV.x > 1 || q_UV.y < 0 || q_UV.y > 1);

	accum_data = float4(accum_data.xyz*accum_data.w + noise.xyz, accum_data.w + 1);
    accum_data = float4(accum_data.xyz / accum_data.w, accum_data.w);

    output_texture[screen_coord] = accum_data;
	variance_out_texture[screen_coord] = variance;
}

[numthreads(16,16,1)]
void shadow_denoiser_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
    float2 screen_size = float2(0.f, 0.f);
	output_texture.GetDimensions(screen_size.x, screen_size.y);

	float2 uv = float2(dispatch_thread_id.x / screen_size.x, dispatch_thread_id.y / screen_size.y);

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y);

    const float depth_f = depth_texture[screen_coord].r;

    const float3 normal_f = normal_texture[screen_coord].xyz;

    const float3 position_f = unpack_position(float2(uv.x, 1.f - uv.y), depth_f, inv_projection, inv_view);

    const float3 color_f = input_texture[screen_coord].xyz;

    float2 center = floor(kernel_size/2.0);

    float4 accum = float4(0, 0, 0, 0);

    float variance = variance_in_texture[screen_coord].r;

    if(variance<0.05)
    {
        output_texture[screen_coord] = float4(color_f.xyz, 1.0);
        return;
    }

    [unroll]
    for(int i = 0; i < 5; ++i)
    {
        [unroll]
        for(int j = 0; j < 5; ++j)
        {
            float2 offset = (float2(i,j) - center)*step_distance;
            float2 t_uv = float2((screen_coord.x + offset.x) / screen_size.x, (screen_coord.y + offset.y) / screen_size.y);

            float3 c_temp = input_texture[offset + screen_coord].xyz;
            float3 t = color_f - c_temp;
            float dist2 = dot(t, t);
            float c_w = min(exp(-(dist2)/(c_phi)), 1.0);

            float3 n_temp = normal_texture[offset + screen_coord];
            t = normal_f - n_temp;
            dist2 = max(dot(t,t)/(step_distance*step_distance),0.0);
            float n_w = min(exp(-(dist2)/0.1), 1.0);

            float t_depth = depth_texture[offset + screen_coord];

            float3 p_temp = unpack_position(float2(t_uv.x, 1.0 - t_uv.y), t_depth, inv_projection, inv_view);
            t = position_f - p_temp;
            dist2 = dot(t, t);
            float p_w = min(exp(-(dist2)/p_phi), 1.0);

            float weight =  c_w * n_w * p_w;
            float kernel = WEIGHTS[i] * WEIGHTS[j];
            accum.xyz += c_temp * kernel * weight;
            accum.w += kernel * weight;
              
        }
    }
    
    output_texture[screen_coord] = accum/accum.w;
}
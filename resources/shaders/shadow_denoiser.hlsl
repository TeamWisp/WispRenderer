Texture2D input_texture : register(t0);
Texture2D depth_texture : register(t1);
Texture2D velocity_texture : register(t2);
Texture2D kernel_texture : register(t3);
Texture2D accum_texture : register(t4);
Texture2D variance_in_texture : register(t5);
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
};

const static float2 VARIANCE_CLIPPING_KERNEL = float2(7, 7);
const static float VARIANCE_CLIPPING_GAMMA = 0.75;

float3 unpack_position(float2 uv, float depth, float4x4 proj_inv, float4x4 view_inv) {
	const float4 ndc = float4(uv * 2.0 - 1.0, depth, 1.0);
	const float4 pos = mul( view_inv, mul(proj_inv, ndc));
	return (pos / pos.w).xyz;
}

float3 line_box_intersection(float3 box_min, float3 box_max, float3 line_start, float3 line_end)
{
    if(line_start.x < box_max.x && line_start.x > box_min.x &&
     line_start.y < box_max.y && line_start.y > box_min.y &&
     line_start.z < box_max.z && line_start.z > box_min.z)
     {
         return line_start;
     }

     float3 ray_dir = normalize(line_end - line_start);
     float3 t = float3(0,0,0);

     for(int i = 0; i< 3; ++i)
     {
        if(ray_dir[i] > 0)
        {
            t[i] = (box_min[i] - line_start[i]) / ray_dir[i];
        }
        else
        {
            t[i] = (box_max[i] - line_start[i]) / ray_dir[i];
        }
     }

     float mt = max(t.x, max(t.y, t.z));

     return line_start + ray_dir*mt;
}

[numthreads(16,16,1)]
void temporal_accumulator_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	output_texture.GetDimensions(screen_size.x, screen_size.y);

	float2 uv = float2(dispatch_thread_id.x / screen_size.x, dispatch_thread_id.y / screen_size.y);

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y);

    float2 q_UV = uv - velocity_texture.SampleLevel(point_sampler, uv, 0);

    //q_UV.y = 1.0 - q_UV.y;

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
    //box_min = min(noise.xyz, min(shadow1, min(shadow2, min(shadow3, shadow4))));
    float3 box_max = min(mu + VARIANCE_CLIPPING_GAMMA * sigma, clamp_max);
    //box_max = max(noise.xyz, max(shadow1, max(shadow2, max(shadow3, shadow4))));

	float mean = accum_data.x / accum_data.w;

    float variance = sigma;

	if(accum_data.w==0.f)
	{
		accum_data = float4(1, 1, 1, 0);
	}

	accum_data.w = clamp(accum_data.w, 1.0, 128.0);

	accum_data.xyz = clamp(accum_data.xyz, box_min, box_max);

    //accum_data.xyz = line_box_intersection(box_min, box_max, accum_data.xyz, noise.xyz);

    accum_data = lerp(accum_data, float4(1,1,1,0), q_UV.x < 0 || q_UV.x > 1 || q_UV.y < 0 || q_UV.y > 1);

	accum_data = float4(accum_data.xyz*accum_data.w + noise.xyz, accum_data.w + 1);
    accum_data = float4(accum_data.xyz / accum_data.w, accum_data.w);

    //bool uv_equal = int2(q_UV.x * screen_size.x,q_UV.y * screen_size.y)==int2(screen_coord.x, screen_coord.y);
    //uv_equal = q_UV.x < 0 || q_UV.x > 1 || q_UV.y < 0 || q_UV.y > 1;

    output_texture[screen_coord] = accum_data;
	variance_out_texture[screen_coord] = variance;
}

[numthreads(16,16,1)]
void shadow_denoiser_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
    float2 screen_size = float2(0.f, 0.f);
	output_texture.GetDimensions(screen_size.x, screen_size.y);

    float2 kernel_image_size = float2(0.f, 0.f);
    kernel_texture.GetDimensions(kernel_image_size.x, kernel_image_size.y);

	float2 uv = float2(dispatch_thread_id.x / screen_size.x, dispatch_thread_id.y / screen_size.y);

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y);

    const float depth_f = depth_texture[screen_coord].r;

    // We are only interested in the depth here
    float4 ndcCoords = float4(0, 0, depth_f, 1.0f);

    // Unproject the vector into (homogenous) view-space vector
    float4 viewCoords = mul(inv_projection, ndcCoords);

    // Divide by w, which results in actual view-space z value
    float linearDepth = viewCoords.z / viewCoords.w;

    float weights = 0.0;

    float accum = 0.0;

    float2 center = floor(kernel_size/2.0);

    float var = variance_in_texture[screen_coord];

    if(var == 0.0)
    {
        accum = input_texture[int2(screen_coord.x, screen_coord.y)];
        weights = 1.0;
    }
    else
    {
        float variance_kernel_size = max(var*5.0, 1.0);
        for(int i = 0; i < int(kernel_size.x); ++i)
        {
            for(int j = 0; j < int(kernel_size.y); ++j)
            {
                float2 kernel_location = float2(i, j);
                float2 coord = float2(kernel_location.x - center.x, kernel_location.y - center.y) * variance_kernel_size + screen_coord;
                if(coord.x < 0 || coord.y < 0 || coord.x > screen_size.x || coord.y > screen_size.y)
                {

                }
                else
                {
                    float shadow_data = input_texture[int2(coord.x, coord.y)].r;
                    float depth_data = depth_texture[int2(coord.x, coord.y)].r;
                    float4 ndcCoords = float4(0, 0, depth_data, 1.0f);
                    float4 viewCoords = mul(inv_projection, ndcCoords);
                    depth_data = viewCoords.z / viewCoords.w;
                    float weight = lerp(
                        kernel_texture.SampleLevel(linear_sampler, float2(kernel_location.x / kernel_size.x, kernel_location.y / kernel_size.y), 0), 
                        0.0,
                        clamp(abs(linearDepth - depth_data)/depth_contrast, 0.0, 1.0));
                    accum += shadow_data*weight;
                    weights += weight;
                }            
            }
        }
    }

    accum /= weights;

    output_texture[screen_coord] = accum;
}
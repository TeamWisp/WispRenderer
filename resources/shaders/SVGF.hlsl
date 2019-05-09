Texture2D input_texture : register(t0);
Texture2D motion_texture : register(t1);
Texture2D normal_texture : register(t2);
Texture2D depth_texture : register(t3);

Texture2D in_hist_length_texture : register(t4);

Texture2D in_variance_texture : register(t5);

Texture2D prev_input_texture : register(t6);
Texture2D prev_moments_texture : register(t7);
Texture2D prev_normal_texture : register(t8);
Texture2D prev_depth_texture : register(t9);

RWTexture2D<float4> out_color_texture : register(u0);
RWTexture2D<float2> out_moments_texture : register(u1);
RWTexture2D<float> out_hist_length_texture : register(u2);
RWTexture2D<float> out_variance_texture : register(u3);

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
	float near_plane;
	float far_plane;
};

cbuffer DenoiserSettings : register(b1)
{
    float c_phi;
    float p_phi;
    float n_phi;
    float step_distance;
};

[numthreads(16,16,1)]
void reprojection_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{

}

[numthreads(16,16,1)]
void filter_moments_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{

}

[numthreads(16,16,1)]
void wavelet_filter_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{

}
#include "util.hlsl"
#include "pbr_util.hlsl"

Texture2D input_texture : register(t0);
Texture2D dir_hitT_texture : register(t1);
Texture2D albedo_roughness_texture : register(t2);
Texture2D normal_metallic_texture : register(t3);
Texture2D normalized_depth_texture : register(t4);
RWTexture2D<float4> output_texture : register(u0);

SamplerState point_sampler   : register(s0);

[numthreads(16, 16, 1)]
void spatial_denoiser_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
    float2 screen_coord = float2(dispatch_thread_id.xy);

    output_texture[screen_coord] = input_texture[screen_coord];
}
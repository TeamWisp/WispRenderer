#ifndef __PP_BLOOM_COMPOSITION_HLSL__
#define __PP_BLOOM_COMPOSITION_HLSL__

#include "pp_hdr_util.hlsl"

Texture2D source_main : register(t0);
Texture2D source_bloom_half : register(t1);
Texture2D source_bloom_qes : register(t2);
RWTexture2D<float4> output : register(u0);
SamplerState linear_sampler : register(s0);
SamplerState point_sampler : register(s1);

cbuffer Bloomproperties : register(b0)
{
	int enable_bloom;
};

[numthreads(16, 16, 1)]
void main_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	output.GetDimensions(screen_size.x, screen_size.y);

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y) + 0.5f;
	float2 texel_size = 1.0f / screen_size;

	float2 uv = screen_coord / screen_size;
	float2 uv_half = (screen_coord / 2 ) / screen_size;
	float2 uv_quarter = (screen_coord / 4 ) / screen_size + 0.5f;
	float2 uv_eighth = (screen_coord / 8) / screen_size + 0.75f;
	float2 uv_sixteenth = (screen_coord / 16 ) / screen_size + 0.875f;

	float3 finalcolor = float3(0, 0, 0);
	float bloom_intensity = 1.f;

	if (enable_bloom > 0)
	{
		finalcolor += source_bloom_half.SampleLevel(linear_sampler, uv, 0).rgb;
		finalcolor += source_bloom_qes.SampleLevel(linear_sampler, uv_half, 0).rgb;
		finalcolor += source_bloom_qes.SampleLevel(linear_sampler, uv_quarter, 0).rgb;
		finalcolor += source_bloom_qes.SampleLevel(linear_sampler, uv_eighth, 0).rgb;
		finalcolor += source_bloom_qes.SampleLevel(linear_sampler, uv_sixteenth, 0).rgb;
		
		finalcolor *= 0.25f;
	}

	finalcolor += source_main.SampleLevel(point_sampler, uv, 0).rgb;

	output[int2(dispatch_thread_id.xy)] = float4(finalcolor, 1.0f);
}

#endif //__PP_BLOOM_COMPOSITION_HLSL__
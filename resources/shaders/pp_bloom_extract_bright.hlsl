#ifndef __PP_BLOOM_EXTRACT_BRIGHT_HLSL__
#define __PP_BLOOM_EXTRACT_BRIGHT_HLSL__

#include "pp_dof_util.hlsl"

Texture2D source : register(t0);
Texture2D g_emissive : register(t1);
Texture2D g_depth : register(t2);

RWTexture2D<float4> output_bright : register(u0);
SamplerState s0 : register(s0);
SamplerState s1 : register(s1);


[numthreads(16, 16, 1)]
void main_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	output_bright.GetDimensions(screen_size.x, screen_size.y);

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y) + 0.5f;
	float2 uv = screen_coord / screen_size;

	float4 out_bright = float4(0.0f, 0.0f, 0.0f, 1.0f);
	float3 final_color = source.SampleLevel(s1, uv, 0).rgb;

	float brightness = dot(final_color, float3(0.2126f, 0.7152f, 0.0722f));

	if (brightness > 1.0f && g_depth.SampleLevel(s1, uv, 0).r < 1)
	{
		out_bright = saturate(float4(final_color, 1.0f));
	}

	out_bright += float4(g_emissive.SampleLevel(s0, uv, 0).rgb, 1.0f);

	output_bright[int2(dispatch_thread_id.xy)] = out_bright;
}

#endif //__PP_BLOOM_EXTRACT_BRIGHT_HLSL__
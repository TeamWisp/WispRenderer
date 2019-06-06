#ifndef __PP_DOF_DILATE_HLSL__
#define __PP_DOF_DILATE_HLSL__

#include "pp_dof_properties.hlsl"
#include "pp_dof_util.hlsl"

Texture2D source_near : register(t0);
RWTexture2D<float4> output_near : register(u0);
SamplerState s0 : register(s0);

[numthreads(16, 16, 1)]
void main_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	output_near.GetDimensions(screen_size.x, screen_size.y);

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y) + 0.5f;

	float2 uv = (screen_coord) / screen_size;
	static const int sample_radius = 2;

	float output = source_near.SampleLevel(s0, uv , 0).a;

	[unroll]
	for (int y = -sample_radius; y <= sample_radius; ++y)
	{
		[unroll]
		for (int x = -sample_radius; x <= sample_radius; ++x)
		{
			output = max(output, source_near.SampleLevel(s0, ((screen_coord + 0.5f + float2(x, y)) / screen_size), 0).a);
		}
	}

	output_near[int2(dispatch_thread_id.xy)] = output;
}

#endif //__PP_DOF_DILATE_HLSL__
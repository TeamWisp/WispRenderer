#include "dof_properties.hlsl"
#include "dof_util.hlsl"

Texture2D source_near : register(t0);
RWTexture2D<float4> output_near : register(u0);
SamplerState s0 : register(s0);

[numthreads(16, 16, 1)]
void main_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	output_near.GetDimensions(screen_size.x, screen_size.y);

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y);
	float2 texel_size = 1.0f / screen_size;
	float2 uv = (screen_coord + 0.5f) / screen_size;
	static const int SampleRadius = 2;
	static const int SampleDiameter = SampleRadius * 2;

	float output = source_near.SampleLevel(s0, uv , 0).a;

	[unroll]
	for (int y = -SampleRadius; y <= SampleRadius; ++y)
	{
		[unroll]
		for (int x = -SampleRadius; x <= SampleRadius; ++x)
		{
			output = max(output, source_near.SampleLevel(s0, ((screen_coord + 0.5f + float2(x, y)) / screen_size), 0).a);
		}
	}

	output_near[int2(dispatch_thread_id.xy)] = output;
}

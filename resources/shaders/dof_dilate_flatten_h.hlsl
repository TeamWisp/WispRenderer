#include "dof_properties.hlsl"
#include "dof_util.hlsl"

Texture2D source_near : register(t0);
RWTexture2D<float4> output_near : register(u0);
SamplerState s0 : register(s0);

[numthreads(16, 16, 1)]
void main_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	source_near.GetDimensions(screen_size.x, screen_size.y);

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y) + 0.5f;
	float2 texel_size = 1.0f / screen_size;

	float2 uv = (screen_coord) / screen_size;

	float sigma = 2.0f;
	float color = 0;
	float weightSum = 0.0f;
	for (int i = -7; i < 7; i++)
	{
		float weight = CalcGaussianWeight(i, sigma);
		weightSum += weight;
		float2 o_uv = (screen_coord + (float2(1.0f * i, 0.0f))) / screen_size;
		float4 s = source_near.SampleLevel(s0, o_uv, 0).xxxx;
		color += s * weight;
	}

	color /= weightSum;

	color = max(source_near.SampleLevel(s0, uv, 0).x, color);
	output_near[int2(dispatch_thread_id.xy)] = color;
}

#include "vignette.hlsl"
#include "hdr_util.hlsl"

Texture2D<float4> input : register(t0);
RWTexture2D<float4> output : register(u0);
SamplerState s0 : register(s0);

cbuffer Properties : register(b0)
{
	float frame_idx;
};


[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float4 current = input[DTid.xy];
	float4 prev = output[DTid.xy];
	if (frame_idx < 1)
	{
		prev = 0;
	}

	float accum_count = frame_idx;

	//float3 color = input.SampleLevel(s0, uv, 0).rgb / (frame_idx + 1);
	float4 color = (accum_count * prev + current) / (accum_count + 1.f);

	output[DTid.xy] = current;
}

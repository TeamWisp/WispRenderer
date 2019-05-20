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
	float4 current = input[DTid.xy]; // Last path tracer result
	float4 prev = output[DTid.xy]; // Previous path tracer output

	float accum_count = frame_idx; // 0 to x, the number of times the accumulation has ran.

	float4 color = (accum_count * prev + current) / (accum_count + 1); // accumulate

	output[DTid.xy] = color; // update the output with the accumulated result.
}

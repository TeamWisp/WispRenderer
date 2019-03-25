#include "vignette.hlsl"
#include "hdr_util.hlsl"

Texture2D<float4> input : register(t0);
RWTexture2D<float4> output : register(u0);
SamplerState s0 : register(s0);

cbuffer CameraProperties : register(b0)
{
	float hdr;
};


float4 ChromaticAberration(Texture2D tex, float2 uv, float strength) {
	float2 r_offset = float2(strength, 0);
	float2 g_offset = float2(-strength, 0);
	float2 b_offset = float2(0, 0);
 
	float4 r = float4(1, 1, 1, 1);
	r.x = tex.SampleLevel(s0, uv + r_offset, 0).x;
	r.y = tex.SampleLevel(s0, uv + g_offset, 0).y;
	r.z = tex.SampleLevel(s0, uv + b_offset, 0).z;
	r.a = tex.SampleLevel(s0, uv, 0).a;
 
	return r;
}

[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float2 resolution;
	input.GetDimensions(resolution.x, resolution.y);
	
	float2 uv = float2(DTid.xy) / resolution;
	float gamma = 2.2;
	float exposure = 1;

	//float3 color = ChromaticAberration(input, uv, 0.001).rgb;
	float3 color = input[DTid.xy].rgb;

	if (hdr == 0)
	{
		color = linearToneMapping(color, exposure, gamma);
		//color = simpleReinhardToneMapping(color, exposure, gamma);
		//color = lumaBasedReinhardToneMapping(color, gamma);
		//color = whitePreservingLumaBasedReinhardToneMapping(color, gamma);
		//color = RomBinDaHouseToneMapping(color, gamma);
		//color = filmicToneMapping(color);
		//color = Uncharted2ToneMapping(color, exposure, gamma);
		//color = GrayscaleToneMapping(color);
		//color = AllTonemappingAlgorithms(color.rgb, uv.x + uv.y, exposure, gamma);
		//color = Vignette(color, uv, 1.5, 0.5, 0.5);
	}

	output[DTid.xy] = float4(color, 1);


}

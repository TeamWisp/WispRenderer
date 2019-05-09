#include "vignette.hlsl"
#include "hdr_util.hlsl"
#include "lens_effects.hlsl"

Texture2D<float4> input : register(t0);
RWTexture2D<float4> output : register(u0);
SamplerState s0 : register(s0);

cbuffer CameraProperties : register(b0)
{
	float hdr;
};

[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float2 resolution;
	input.GetDimensions(resolution.x, resolution.y);
	
	float2 uv = (float2(DTid.xy + 0.5f) / resolution);

	float gamma = 2.2;
	float exposure = 1;

	float3 color = input.SampleLevel(s0, uv, 0).rgb;
	color = SampleFXAA(input, s0, DTid.xy + 0.5f, resolution);
	//uv = ZoomUV(uv, 0.75);
	//float3 color = input.SampleLevel(s0, BarrelDistortUV(uv, 2), 0);
	//float3 color = ChromaticAberrationV2(input, s0, uv, 0.2, 0.96f).rgb;

	if (hdr == 0)
	{
		color = linearToneMapping(color, exposure, gamma);
		//color = simpleReinhardToneMapping(color, exposure, gamma);
		//color = lumaBasedReinhardToneMapping(color, gamma);
		//color = whitePreservingLumaBasedReinhardToneMapping(color, gamma);
		//color = RomBinDaHouseToneMapping(color, gamma);
		//color = filmicToneMapping(color);
		//color = Uncharted2ToneMapping(color, gamma, exposure);
		//color = GrayscaleToneMapping(color);
		//color = AllTonemappingAlgorithms(color.rgb, uv.x + uv.y, exposure, gamma);
		//color = Vignette(color, uv, 1.5, 0.5, 0.5);
	}

	output[DTid.xy] = float4(color, 1);
}

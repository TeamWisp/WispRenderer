/*!
 * Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __PP_TONEMAPPING_HLSL__
#define __PP_TONEMAPPING_HLSL__

#include "pp_util.hlsl"
#include "pp_hdr_util.hlsl"

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
	//color = SampleFXAA(input, s0, DTid.xy + 0.5f, resolution);
	//uv = ZoomUV(uv, 0.75);
	//float3 color = input.SampleLevel(s0, BarrelDistortUV(uv, 2), 0);
	//float3 color = ChromaticAberrationV2(input, s0, uv, 0.2, 0.96f).rgb;

	if (hdr == 0)
	{
		//color = linearToneMapping(color, exposure, gamma);
		//color = simpleReinhardToneMapping(color, exposure, gamma);
		//color = lumaBasedReinhardToneMapping(color, gamma);
		//color = whitePreservingLumaBasedReinhardToneMapping(color, gamma);
		//color = RomBinDaHouseToneMapping(color, gamma);
		//color = filmicToneMapping(color);
		//color = Uncharted2ToneMapping(color, gamma, exposure);
		//color = GrayscaleToneMapping(color);
		color = ACESToneMapping(color, exposure, gamma);
		//color = AllTonemappingAlgorithms(color.rgb, uv.x + uv.y, exposure, gamma);
		//color = Vignette(color, uv, 1.5, 0.5, 0.5);
	}

	output[DTid.xy] = float4(color, 1);
}

#endif //__PP_TONEMAPPING_HLSL__

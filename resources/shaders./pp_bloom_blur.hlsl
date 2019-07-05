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
#ifndef __PP_BLOOM_BLUR_HLSL__
#define __PP_BLOOM_BLUR_HLSL__

#include "pp_dof_util.hlsl"
#include "pp_bloom_util.hlsl"

Texture2D source : register(t0);
RWTexture2D<float4> output : register(u0);
SamplerState s0 : register(s0);

cbuffer BloomDirection : register(b0)
{
	int blur_direction;
	float sigma_amt;
};

[numthreads(16, 16, 1)]
void main_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	output.GetDimensions(screen_size.x, screen_size.y);

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y) + 0.5f;
	float2 texel_size = 1.0f / screen_size;

	float2 uv = (screen_coord) / screen_size;

	float sigma = sigma_amt - 1.0f;

	float2 blur_dir = float2(0.0f, 1.0f);

	if (blur_direction == 1)
	{
		blur_dir = float2(1.0f, 0.0f);
	}

	float4 color = 0;
	float weightSum = 0.0f;
	for (int i = -7; i < 7; i++)
	{
		float weight = CalcGaussianWeight(i, sigma);
		weightSum += weight;
		float2 o_uv = saturate((screen_coord + (blur_dir * i)) / screen_size);
		float4 s = source.SampleLevel(s0, o_uv, 0);
		color += s * weight;
	}

	output[int2(dispatch_thread_id.xy)] = color;
}

#endif //__PP_BLOOM_BLUR_HLSL__
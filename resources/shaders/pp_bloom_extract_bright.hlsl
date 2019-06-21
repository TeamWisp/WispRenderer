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

	for (int i = -1; i < 2; ++i)
	{
		uv = float2(screen_coord.x + i, screen_coord.y + i) / screen_size;

		if (brightness > 1.0f && g_depth.SampleLevel(s1, uv, 0).r < 1)
		{
			out_bright = saturate(float4(final_color, 1.0f));
		}

		out_bright += float4(g_emissive.SampleLevel(s0, uv, 0).rgb, 1.0f);
	}

	out_bright /= 3;

	output_bright[int2(dispatch_thread_id.xy)] = out_bright;
}

#endif //__PP_BLOOM_EXTRACT_BRIGHT_HLSL__
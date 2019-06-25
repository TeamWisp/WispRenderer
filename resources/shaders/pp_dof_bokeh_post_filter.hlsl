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
#ifndef __PP_DOF_BOKEH_POST_FILTER_HLSL__
#define __PP_DOF_BOKEH_POST_FILTER_HLSL__

Texture2D source_near : register(t0);
Texture2D source_far : register(t1);
RWTexture2D<float4> output_near : register(u0);
RWTexture2D<float4> output_far : register(u1);
SamplerState s0 : register(s0);
SamplerState s1 : register(s1);

[numthreads(16, 16, 1)]
void main_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	output_near.GetDimensions(screen_size.x, screen_size.y);
	screen_size -= 1.0f;

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y);


	float2 uv = screen_coord / screen_size;

	float nearcoc = source_near.SampleLevel(s1, uv, 0).a;
	float farcoc = source_far.SampleLevel(s1, uv, 0).a;

	static const int SampleRadius = 2;
	static const int SampleDiameter = SampleRadius * 2 + 1;

	float3 near_color = 0;
	float3 far_color = 0;

	[unroll]
	for (int y = -SampleRadius; y <= SampleRadius; ++y)
	{
		[unroll]
		for (int x = -SampleRadius; x <= SampleRadius; ++x)
		{
			near_color += source_near.SampleLevel(s0, (screen_coord + float2(x, y)) / screen_size, 0).rgb;
			far_color += source_far.SampleLevel(s0, (screen_coord + float2(x, y)) / screen_size, 0).rgb;
		}
	}

	near_color /= float(SampleDiameter * SampleDiameter);
	far_color /= float(SampleDiameter * SampleDiameter);

	output_near[int2(dispatch_thread_id.xy)] = float4(near_color, nearcoc);
	output_far[int2(dispatch_thread_id.xy)] = float4(far_color, farcoc);
}

#endif //__PP_DOF_BOKEH_POST_FILTER_HLSL__
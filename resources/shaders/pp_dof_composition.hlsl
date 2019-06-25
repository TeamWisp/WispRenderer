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
#ifndef __PP_DOF_COMPOSITION_HLSL__
#define __PP_DOF_COMPOSITION_HLSL__

#include "pp_dof_properties.hlsl"

Texture2D source : register(t0);
RWTexture2D<float4> output : register(u0);
Texture2D bokeh_near : register(t1);
Texture2D bokeh_far : register(t2);
Texture2D coc_buffer : register(t3);
SamplerState s0 : register(s0);
SamplerState s1 : register(s1);

float GetDownSampledCoC(float2 uv , float2 texel_size)
{
	float4 offset = texel_size.xyxy * float2(-0.5f, 0.5f).xxyy;
	float coc0 = coc_buffer.SampleLevel(s1, uv + offset.xy, 0);
	float coc1 = coc_buffer.SampleLevel(s1, uv + offset.zy, 0);
	float coc2 = coc_buffer.SampleLevel(s1, uv + offset.xw, 0);
	float coc3 = coc_buffer.SampleLevel(s1, uv + offset.zw, 0);

	float coc_min = min(min(min(coc0, coc1), coc2), coc3);
	float coc_max = max(max(max(coc0, coc1), coc2), coc3);

	float coc = coc_max >= -coc_min ? coc_max : coc_min;
	return coc;
}

[numthreads(16, 16, 1)]
void main_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	output.GetDimensions(screen_size.x, screen_size.y);
	screen_size -= 1.0f;

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y);


	float2 texel_size = 1.0 / screen_size;
	float2 uv = (screen_coord) / (screen_size);

	float coc = coc_buffer.SampleLevel(s0, uv, 0);
	
	float3 original_sample = source.SampleLevel(s0, uv, 0).rgb;
	float4 near_sample = bokeh_near.SampleLevel(s1, uv, 0);
	float4 far_sample = bokeh_far.SampleLevel(s1, uv, 0);

	float near_w = bokeh_near.SampleLevel(s0, uv, 0).a;

	float3 near = near_sample.rgb;
	float3 far = original_sample.rgb; 

	if (far_sample.w > 0.0f)
	{
		far = far_sample.rgb / far_sample.w;
	}

	float far_blend = saturate(saturate(coc) * MAXCOCSIZE - 0.5f);

	float3 result = lerp(original_sample, far.rgb, far_blend);

	float near_blend = saturate(near_sample.w * 2.0f);

	result = lerp(result, near.rgb, smoothstep(0.0f, 1.0f, near_blend));

	output[int2(dispatch_thread_id.xy)] = float4(result, coc);
}

#endif //__PP_DOF_COMPOSITION_HLSL__
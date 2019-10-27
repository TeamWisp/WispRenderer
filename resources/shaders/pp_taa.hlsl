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
#ifndef __PP_TAA_HLSL__
#define __PP_TAA_HLSL__

Texture2D input_texture : register(t0);
Texture2D motion_texture : register(t1);
Texture2D accum_texture : register(t2);
Texture2D history_in_texture : register(t3);

RWTexture2D<float4> output_texture : register(u0);
RWTexture2D<float> history_out_texture : register(u1);

const static float VARIANCE_CLIPPING_GAMMA = 1.0;

float4 LineBoxIntersection(float3 box_min, float3 box_max, float3 c_in, float3 c_hist)
{
    float3 p_clip = 0.5 * (box_max + box_min);
    float3 e_clip = 0.5 * (box_max - box_min);

    float3 v_clip = c_hist - p_clip;
    float3 v_unit = v_clip.xyz / e_clip;
    float3 a_unit = abs(v_unit);
    float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));

    if(ma_unit > 1.0)
    {
        return float4((p_clip + v_clip / ma_unit).xyz, ma_unit);
    }
    else
    {
        return float4(c_hist.xyz, ma_unit);
    }
}

[numthreads(16,16,1)]
void main_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	output_texture.GetDimensions(screen_size.x, screen_size.y);

	float2 uv = float2(dispatch_thread_id.x / screen_size.x, dispatch_thread_id.y / screen_size.y);

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y);

	float2 motion = motion_texture[screen_coord].xy;

	float2 q_uv = uv - motion;

	float2 prev_coord = q_uv*screen_size;

	bool success = prev_coord.x >= 0 && prev_coord.x < screen_size.x && prev_coord.y >= 0 && prev_coord.y < screen_size.y;

	float hist = history_in_texture[prev_coord].x;

	float3 input = input_texture[screen_coord].xyz;
	float3 accum = accum_texture[screen_coord].xyz;

	hist = min(16, success ? (hist + 1) : 1);

	float3 moment_1 = float3(0.0, 0.0, 0.0);
	float3 moment_2 = float3(0.0, 0.0, 0.0);

	float3 clamp_min = 1.0;
	float3 clamp_max = 0.0;

	[unroll]
	for(int y = -2; y <= 2; ++y)
	{
		[unroll]
		for(int x = -2; x <= 2; ++x)
		{
			float3 color = input_texture[screen_coord + int2(x, y)].xyz;
			moment_1 += color;
			moment_2 += color * color;
			clamp_min = min(color, clamp_min);
			clamp_max = max(color, clamp_max);
		}
	}

	float3 mu = moment_1 / 25.0;
	float3 sigma = sqrt(moment_2 / 25.0 - mu*mu);

	float3 box_min = max(mu - VARIANCE_CLIPPING_GAMMA * sigma, clamp_min);
	float3 box_max = min(mu + VARIANCE_CLIPPING_GAMMA * sigma, clamp_max);

	float4 clipped = LineBoxIntersection(box_min, box_max, input, accum);

	const float alpha = lerp(1.0, max(0.05f, 1.0/hist), success);

	history_out_texture[screen_coord] = hist;

	input = lerp(clipped.xyz, input, alpha);

	output_texture[screen_coord] = float4(input.xyz, 1.0f);
}

#endif // __PP_TAA_HLSL__
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
#ifndef __PP_DOF_COC_HLSL__
#define __PP_DOF_COC_HLSL__

#include "pp_dof_properties.hlsl"
#include "pp_dof_util.hlsl"

Texture2D gbuffer_depth : register(t0);
RWTexture2D<float2> output : register(u0);
SamplerState s0 : register(s0);

cbuffer CameraProperties : register(b0)
{
	float4x4 projection;
	float focal_length;
	float f_number;
	float film_size;
	float focus_dist;
	float2 m_pad;
	int enable_dof;
	float dof_range;
};

float GetCoC(float lineardepth, float focusdist)
{
	float2 screen_size = float2(0.f, 0.f);
	output.GetDimensions(screen_size.x, screen_size.y);

	float fstop = focal_length / f_number * 0.5f;
	////// Compute CoC in meters
	float coc = -fstop * (focal_length * (focusdist - lineardepth)) / (lineardepth * (focusdist - focal_length));

	//// Convert to pixels
	coc = (coc / film_size) * screen_size.x;

	coc = clamp(coc / (MAXCOCSIZE * dof_range), -1.f, 1.f);
	return coc;
}


float GetAutoFocusDepth(float2 screen_dimensions)
{
	float depth_focus = gbuffer_depth[float2(0.5f * screen_dimensions.x, 0.5f * screen_dimensions.y)].r;

	depth_focus += gbuffer_depth[float2(0.51f * screen_dimensions.x, 0.51f * screen_dimensions.y)].r;
	depth_focus += gbuffer_depth[float2(0.49f * screen_dimensions.x, 0.51f * screen_dimensions.y)].r;
	depth_focus += gbuffer_depth[float2(0.51f * screen_dimensions.x, 0.49f * screen_dimensions.y)].r;
	depth_focus += gbuffer_depth[float2(0.49f * screen_dimensions.x, 0.49f * screen_dimensions.y)].r;

	depth_focus += gbuffer_depth[float2(0.52f * screen_dimensions.x, 0.52f * screen_dimensions.y)].r;
	depth_focus += gbuffer_depth[float2(0.48f * screen_dimensions.x, 0.52f * screen_dimensions.y)].r;
	depth_focus += gbuffer_depth[float2(0.52f * screen_dimensions.x, 0.48f * screen_dimensions.y)].r;
	depth_focus += gbuffer_depth[float2(0.48f * screen_dimensions.x, 0.48f * screen_dimensions.y)].r;

	depth_focus = (depth_focus / 9.0f);

	return depth_focus;
}

[numthreads(16, 16, 1)]
void main_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	output.GetDimensions(screen_size.x, screen_size.y);
	screen_size -= 1.0f;

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y);

	float2 uv = screen_coord / screen_size;
	
	float sample_depth = gbuffer_depth.SampleLevel(s0, uv, 0).r;
	float focus_depth = GetAutoFocusDepth(screen_size);

	sample_depth = GetLinearDepth(sample_depth) * FFAR;

	float coc = GetCoC(sample_depth, focus_dist);

	if (focus_dist < 1)
	{
		focus_depth = GetLinearDepth(focus_depth) * FFAR;
		coc = GetCoC(sample_depth, focus_depth);
	}
	if (enable_dof == 0)
	{
		coc = 0.0f;
	}

	float2 result = float2(coc, gbuffer_depth.SampleLevel(s0, uv, 0).r);
	output[int2(dispatch_thread_id.xy)] = result;
}

#endif //__PP_DOF_COC_HLSL__
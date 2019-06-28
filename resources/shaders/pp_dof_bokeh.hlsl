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
#ifndef __PP_DOF_BOKEH_HLSL__
#define __PP_DOF_BOKEH_HLSL__

#include "pp_dof_properties.hlsl"
#include "pp_dof_util.hlsl"

Texture2D source_near : register(t0);
Texture2D source_far : register(t1);
Texture2D near_mask :register(t2);
RWTexture2D<float4> output_near : register(u0);
RWTexture2D<float4> output_far : register(u1);
SamplerState s0 : register(s0);
SamplerState s1 : register(s1);

cbuffer CameraProperties : register(b0)
{
	float f_number;
	float shape_curve;
	float bokeh_poly_amount;
	uint num_blades;
	float m_padding;
	float2 m_bokeh_shape_modifier;
	int enable_dof;
};


[numthreads(16, 16, 1)]
void main_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	source_near.GetDimensions(screen_size.x, screen_size.y);
	screen_size -= 1.0f;

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y);

	float2 texel_size = 1.0f / screen_size;

	float2 uv = screen_coord / (screen_size) ;

	const uint NUMSAMPLES = NUMDOFSAMPLES * NUMDOFSAMPLES;
	const float MAXKERNELSIZE = MAXBOKEHSIZE * 0.5f;
	const float SHAPECURVE = 2.0f;

	float4 fgcolor = float4(0, 0, 0, 0);
	float4 bgcolor = float4(0, 0, 0, 0);

	//Kernel gather method credits to Matt Pettineo and David Neubelt.
	if (enable_dof > 0)
	{
		float far_coc = source_far.SampleLevel(s1, uv, 0).w;
		float near_coc = source_near.SampleLevel(s1, uv, 0).w;
		float kernel_radius = MAXKERNELSIZE * far_coc;

		[branch]
		if (kernel_radius > 0.5f)
		{
			float weightsum = 0.0001f;
			[unroll]
			for (uint i = 0; i < NUMSAMPLES; ++i)
			{
				float lensX = saturate((i % NUMDOFSAMPLES) / max(NUMDOFSAMPLES - 1.0f, 1.0f));
				float lensY = saturate((i / NUMDOFSAMPLES) / max(NUMDOFSAMPLES - 1.0f, 1.0f));
				float2 kernel_offset = SquareToConcentricDiskMapping(lensX, lensY, float(num_blades), bokeh_poly_amount) * m_bokeh_shape_modifier;
				float4 s = source_far.SampleLevel(s0, (screen_coord + kernel_offset * kernel_radius) / screen_size, 0.0f);
				float samplecoc = s.w;

				s *= saturate(1.0f + (samplecoc - far_coc));
				s *= (1.0f - shape_curve) + pow(max(length(kernel_offset), 0.01f), SHAPECURVE) * shape_curve;

				bgcolor += s;
			}

			bgcolor /= NUMSAMPLES;
		}
		else
		{
			bgcolor = source_far.SampleLevel(s0, uv, 0);
		}

		float nearMask = SampleTextureBSpline(near_mask, s0, uv).x;
		nearMask = saturate(nearMask * 1.0f);
		near_coc = max(near_coc, nearMask);
		kernel_radius = near_coc * MAXKERNELSIZE;
		[branch]
		if (kernel_radius > 0.25f)
		{
			float weightsum = 0.0001f;

			[unroll]
			for (uint i = 0; i < NUMSAMPLES; ++i)
			{
				float lensX = saturate((i % NUMDOFSAMPLES) / max(NUMDOFSAMPLES - 1.0f, 1.0f));
				float lensY = saturate((i / NUMDOFSAMPLES) / max(NUMDOFSAMPLES - 1.0f, 1.0f));

				float2 kernel_offset = SquareToConcentricDiskMapping(lensX, lensY, float(num_blades), bokeh_poly_amount) * m_bokeh_shape_modifier;
				float4 s = source_near.SampleLevel(s0, (screen_coord + kernel_offset * kernel_radius) / screen_size , 0.0f);
				float samplecoc = saturate(s.w * MAXKERNELSIZE);

				fgcolor.xyz += s.xyz * s.w;

				float samplealpha = 1.0f;
				samplealpha *= saturate(s.w * 1.0f);
				fgcolor.w += samplealpha;

				weightsum += s.w;
			}

			fgcolor.xyz /= weightsum;
			fgcolor.w = saturate(fgcolor.w * (1.0f / NUMSAMPLES));
			fgcolor.w = max(fgcolor.w, source_near.SampleLevel(s0, uv, 0).w);
		}
		else
		{
			fgcolor = float4(source_near.SampleLevel(s1, uv, 0).rgb, 0.0f);
		}
	}

	output_near[int2(dispatch_thread_id.xy)] = fgcolor;
	output_far[int2(dispatch_thread_id.xy)] = bgcolor;
}

#endif //__PP_DOF_BOKEH_HLSL__
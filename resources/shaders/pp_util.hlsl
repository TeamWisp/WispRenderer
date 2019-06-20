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
#ifndef __PP_UTIL_HLSL__
#define __PP_UTIL_HLSL__

#include "pp_fxaa.hlsl"
#include "math.hlsl"

float3 Vignette(float3 input, float2 pos, float radius, float softness, float strength)
{
	float len = length(pos * 2 - 1);
	float vignette = smoothstep(radius, radius -  softness, len);
	return lerp(input, input * vignette, strength);
}


// Old lens_effects.hlsl file
float2 barrelDistortion(float2 coord, float amt) {
	float2 cc = coord - 0.5;
	float dist = dot(cc, cc);
	return coord + cc * dist * amt;
}

static const int num_iter = 11;
static const float reci_num_iter_f = 1.0 / float(num_iter);

// Old naive chromatic aberration.
float4 ChromaticAberration(Texture2D tex, SamplerState s, float2 uv, float strength) {
	float2 r_offset = float2(strength, 0);
	float2 g_offset = float2(-strength, 0);
	float2 b_offset = float2(0, 0);

	float4 r = float4(1, 1, 1, 1);
	r.x = tex.SampleLevel(s, uv + r_offset, 0).x;
	r.y = tex.SampleLevel(s, uv + g_offset, 0).y;
	r.z = tex.SampleLevel(s, uv + b_offset, 0).z;
	r.a = tex.SampleLevel(s, uv, 0).a;

	return r;
}

// Zoom into a image.
float2 ZoomUV(float2 uv, float zoom)
{
	return (uv * zoom) + ((1 - (zoom)) / 2);
}

float4 ChromaticAberrationV2(Texture2D tex, SamplerState s, float2 uv, float strength, float zoom) {
	uv = ZoomUV(uv, zoom);
	float4 sumcol = 0.0;
	float4 sumw = 0.0;

	float2 resolution;
	tex.GetDimensions(resolution.x, resolution.y);

	for (int i = 0; i < num_iter; ++i)
	{
		float t = float(i) * reci_num_iter_f;
		float4 w = spectrum_offset(t);
		sumw += w;
		//sumcol += w * tex.SampleLevel(s, barrelDistortion(uv, 0.6 * strength*t ), 0);
		sumcol += w * SampleFXAA(tex, s, barrelDistortion(uv, 0.6 * strength * t) * resolution, resolution);
	}

	return sumcol / sumw;
}

float2 BarrelDistortUV(float2 uv, float kcube)
{
	float k = -0.15;

	float r2 = (uv.x - 0.5) * (uv.x - 0.5) + (uv.y - 0.5) * (uv.y - 0.5);
	float f = 0;

	//only compute the cubic distortion if necessary
	if (kcube == 0.0)
	{
		f = 1 + r2 * k;
	}
	else
	{
		f = 1 + r2 * (k + kcube * sqrt(r2));
	};

	// get the right pixel for the current position
	float x = f * (uv.x - 0.5) + 0.5;
	float y = f * (uv.y - 0.5) + 0.5;

	return float2(x, y);
}

#endif //__PP_UTIL_HLSL__
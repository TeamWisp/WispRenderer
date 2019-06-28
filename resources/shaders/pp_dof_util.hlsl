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
#ifndef __PP_DOF_UTIL_HLSL__
#define __PP_DOF_UTIL_HLSL__

static const float FNEAR = 0.1f;
static const float FFAR = 10000.f;

static const float PI = 3.141592654f;
static const float PI2 = 6.283185308f;

float WeighCoC(float coc, float radius)
{
	//return coc >= radius;
	return saturate((coc - radius + 2) / 2);
}

float WeighColor(float3 color)
{
	return 1 / (1 + max(max(color.r, color.g), color.b));
}

float GetLinearDepth(float depth)
{
	float z = (2 * FNEAR) / (FFAR + FNEAR - (depth * (FFAR - FNEAR)));
	return z;
}

// Calculates the gaussian blur weight for a given distance and sigmas
float CalcGaussianWeight(int sampleDist, float sigma)
{
	float g = 1.0f / sqrt(2.0f * 3.14159 * sigma * sigma);
	return (g * exp(-(sampleDist * sampleDist) / (2 * sigma * sigma)));
}


// ------------------------------------------------------------------------------------------------
// Samples a texture with B-spline (bicubic) filtering
// ------------------------------------------------------------------------------------------------
float4 SampleTextureBSpline(in Texture2D textureMap, in SamplerState linearSampler, in float2 uv) {
	float2 texSize;
	textureMap.GetDimensions(texSize.x, texSize.y);
	float2 invTexSize = 1.0f / texSize;

	float2 a = frac(uv * texSize - 0.5f);
	float2 a2 = a * a;
	float2 a3 = a2 * a;
	float2 w0 = (1.0f / 6.0f) * (-a3 + 3 * a2 - 3 * a + 1);
	float2 w1 = (1.0f / 6.0f) * (3 * a3 - 6 * a2 + 4);
	float2 w2 = (1.0f / 6.0f) * (-3 * a3 + 3 * a2 + 3 * a + 1);
	float2 w3 = (1.0f / 6.0f) * a3;
	float2 g0 = w0 + w1;
	float2 g1 = w2 + w3;
	float2 h0 = 1.0f - (w1 / (w0 + w1)) + a;
	float2 h1 = 1.0f - (w3 / (w2 + w3)) - a;

	float2 ex = float2(invTexSize.x, 0.0f);
	float2 ey = float2(0.0f, invTexSize.y);

	w0 = 0.5f;
	w1 = 0.5f;
	g0 = 0.5f;

	float2 uv10 = uv + h0.x * ex;
	float2 uv00 = uv - h1.x * ex;
	float2 uv11 = uv10 + h0.y * ey;
	float2 uv01 = uv00 + h0.y * ey;
	uv10 = uv10 - h1.y * ey;
	uv00 = uv00 - h1.y * ey;

	uv00 = uv + float2(-0.75f, -0.75f) * invTexSize;
	uv10 = uv + float2(0.75f, -0.75f) * invTexSize;
	uv01 = uv + float2(-0.75f, 0.75f) * invTexSize;
	uv11 = uv + float2(0.75f, 0.75f) * invTexSize;

	float4 sample00 = textureMap.SampleLevel(linearSampler, uv00, 0.0f);
	float4 sample10 = textureMap.SampleLevel(linearSampler, uv10, 0.0f);
	float4 sample01 = textureMap.SampleLevel(linearSampler, uv01, 0.0f);
	float4 sample11 = textureMap.SampleLevel(linearSampler, uv11, 0.0f);

	sample00 = lerp(sample00, sample01, g0.y);
	sample10 = lerp(sample10, sample11, g0.y);
	return lerp(sample00, sample10, g0.x);
}

// Maps a value inside the square [0,1]x[0,1] to a value in a disk of radius 1 using concentric squares.
// This mapPIng preserves area, bi continuity, and minimizes deformation.
// Based off the algorithm "A Low Distortion Map Between Disk and Square" by Peter Shirley and
// Kenneth Chiu. Also includes polygon morphing modification from "CryEngine3 Graphics Gems"
// by Tiago Sousa
float2 SquareToConcentricDiskMapping(float x, float y, float numSides, float polygonAmount)
{
	float phi, r;

	// -- (a,b) is now on [-1,1]ˆ2
	float a = 2.0f * x - 1.0f;
	float b = 2.0f * y - 1.0f;

	if (a > -b)                      // region 1 or 2
	{
		if (a > b)                   // region 1, also |a| > |b|
		{
			r = a;
			phi = (PI / 4.0f) * (b / a);
		}
		else                        // region 2, also |b| > |a|
		{
			r = b;
			phi = (PI / 4.0f) * (2.0f - (a / b));
		}
	}
	else                            // region 3 or 4
	{
		if (a < b)                   // region 3, also |a| >= |b|, a != 0
		{
			r = -a;
			phi = (PI / 4.0f) * (4.0f + (b / a));
		}
		else                        // region 4, |b| >= |a|, but a==0 and b==0 could occur.
		{
			r = -b;
			if (abs(b) > 0.0f)
				phi = (PI / 4.0f) * (6.0f - (a / b));
			else
				phi = 0;
		}
	}

	const float N = numSides;
	float polyModifier = cos(PI / N) / cos(phi - (PI2 / N) * floor((N * phi + PI) / PI2));
	r *= lerp(1.0f, polyModifier, polygonAmount);

	float2 result;
	result.x = r * cos(phi);
	result.y = r * sin(phi);

	return result;
}

#endif //__PP_DOF_UTIL_HLSL__
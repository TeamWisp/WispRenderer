#include "dof_properties.hlsl"
#include "dof_util.hlsl"

Texture2D source_near : register(t0);
Texture2D source_far : register(t1);
Texture2D near_mask :register(t2);
RWTexture2D<float4> output_near : register(u0);
RWTexture2D<float4> output_far : register(u1);
SamplerState s0 : register(s0);
SamplerState s1 : register(s1);

static const float PI = 3.141592654f;
static const float PI2 = 6.283185308f;

cbuffer CameraProperties : register(b0)
{
	float f_number;
	float shape_curve;
	float bokeh_poly_amount;
	uint num_blades;
	int enable_dof;
};

float GetDownSampledCoC(float2 uv, float2 texel_size)
{
	float4 offset = texel_size.xyxy * float2(-0.5f, 0.5f).xxyy;

	float coc0 = near_mask.SampleLevel(s1, uv + offset.xy, 0);
	float coc1 = near_mask.SampleLevel(s1, uv + offset.zy, 0);
	float coc2 = near_mask.SampleLevel(s1, uv + offset.xw, 0);
	float coc3 = near_mask.SampleLevel(s1, uv + offset.zw, 0);

	float coc_min = min(min(min(coc0, coc1), coc2), coc3);
	float coc_max = max(max(max(coc0, coc1), coc2), coc3);

	float coc = coc_max >= -coc_min ? coc_max : coc_min;
//	float coc = (coc0 + coc1 + coc2 + coc3) * 0.25f;
	return coc;
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

	float4 sample00 = textureMap.SampleLevel(s0, uv00, 0.0f);
	float4 sample10 = textureMap.SampleLevel(s0, uv10, 0.0f);
	float4 sample01 = textureMap.SampleLevel(s0, uv01, 0.0f);
	float4 sample11 = textureMap.SampleLevel(s0, uv11, 0.0f);

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

[numthreads(16, 16, 1)]
void main_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	source_near.GetDimensions(screen_size.x, screen_size.y);

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y);
	float2 texel_size = 1.0f / screen_size;

	float2 uv = (screen_coord) / screen_size;

	const uint NUMSAMPLES = NUMDOFSAMPLES * NUMDOFSAMPLES;
	const float MAXKERNELSIZE = MAXBOKEHSIZE * 0.5f;
	const float SHAPECURVE = 2.0f;

	float4 fgcolor = float4(0, 0, 0, 0);
	float4 bgcolor = float4(0, 0, 0, 0);


	float masking = 0;
	if (enable_dof > 0)
	{
		//BakingLab kernel gather method credits to MJP and David Neubelt.

		float far_coc = source_far[screen_coord + 0.5f].w;
		float kernel_radius = MAXKERNELSIZE * far_coc;
		

		[branch]
		if (kernel_radius > 0.5f)
		{
			[unroll]
			for (uint i = 0; i < NUMSAMPLES; ++i)
			{
				float lensX = saturate((i % NUMDOFSAMPLES) / max(NUMDOFSAMPLES - 1.0f, 1.0f));
				float lensY = saturate((i / NUMDOFSAMPLES) / max(NUMDOFSAMPLES - 1.0f, 1.0f));
				float2 kernel_offset = SquareToConcentricDiskMapping(lensX, lensY, float(num_blades), bokeh_poly_amount);
				float4 s = source_far.SampleLevel(s0, (screen_coord + 0.5f + kernel_offset * kernel_radius) / screen_size, 0.0f);
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

		float near_coc = source_near[screen_coord + 0.5f].w;
		float nearMask = SampleTextureBSpline(near_mask, s0, uv).x;
		nearMask = saturate(nearMask * 1.0f);
		near_coc = max(near_coc, nearMask);
		masking = nearMask;
		kernel_radius = MAXKERNELSIZE * near_coc;

		[branch]
		if (kernel_radius > 0.25f)
		{
			float weightsum = 0.00001f;

			[unroll]
			for (uint i = 0; i < NUMSAMPLES; ++i)
			{
				float lensX = saturate((i % NUMDOFSAMPLES) / max(NUMDOFSAMPLES - 1.0f, 1.0f));
				float lensY = saturate((i / NUMDOFSAMPLES) / max(NUMDOFSAMPLES - 1.0f, 1.0f));
				float2 kernel_offset = SquareToConcentricDiskMapping(lensX, lensY, float(num_blades), bokeh_poly_amount);
				float4 s = source_near.SampleLevel(s0, (screen_coord + 0.5f + kernel_offset * kernel_radius) / screen_size , 0.0f);
				float samplecoc = s.w * MAXKERNELSIZE;

				float sw = 1.0f;  

				fgcolor.xyz += s.xyz * sw;

				float sampledist = length(kernel_offset) * kernel_radius;

				float samplealpha = 1.0f;
				samplealpha *= saturate(samplecoc * 1.0f);
				fgcolor.w += samplealpha * sw;

				weightsum += sw;
			}

			fgcolor.xyz /= weightsum;
			fgcolor.w = saturate(fgcolor.w * (1.0f / NUMSAMPLES));
			fgcolor.w = max(fgcolor.w, source_near[screen_coord + 0.5f].w);
		}
		else
		{
			fgcolor = source_near.SampleLevel(s0, uv, 0);
		}
		
	}

	//output_near[int2(dispatch_thread_id.xy)] = float4(masking, masking, masking, masking);
	output_near[int2(dispatch_thread_id.xy)] = fgcolor;
	output_far[int2(dispatch_thread_id.xy)] = bgcolor;
}

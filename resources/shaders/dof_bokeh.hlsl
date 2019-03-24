#include "dof_properties.hlsl"

Texture2D source : register(t0);
Texture2D cocbuffer :register(t1);
RWTexture2D<float4> output : register(u0);
SamplerState s0 : register(s0);
SamplerState s1 : register(s1);

static const float Pi = 3.141592654f;
static const float Pi2 = 6.283185308f;

cbuffer CameraProperties : register(b0)
{
	float f_number;
	float bokeh_poly_amount;
	uint num_blades;
	int enable_dof;
};

float GetDownSampledCoC(float2 uv, float2 texelSize)
{
	float4 offset = texelSize.xyxy * float2(-0.5f, 0.5f).xxyy;
	float coc0 = cocbuffer.SampleLevel(s1, uv + offset.xy, 0).a;
	float coc1 = cocbuffer.SampleLevel(s1, uv + offset.zy, 0).a;
	float coc2 = cocbuffer.SampleLevel(s1, uv + offset.xw, 0).a;
	float coc3 = cocbuffer.SampleLevel(s1, uv + offset.zw, 0).a;

	float cocMin = min(min(min(coc0, coc1), coc2), coc3);
	float cocMax = max(max(max(coc0, coc1), coc2), coc3);

	float coc = cocMax >= -cocMin ? cocMax : cocMin;

	return coc;
}

// Maps a value inside the square [0,1]x[0,1] to a value in a disk of radius 1 using concentric squares.
// This mapping preserves area, bi continuity, and minimizes deformation.
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
			phi = (Pi / 4.0f) * (b / a);
		}
		else                        // region 2, also |b| > |a|
		{
			r = b;
			phi = (Pi / 4.0f) * (2.0f - (a / b));
		}
	}
	else                            // region 3 or 4
	{
		if (a < b)                   // region 3, also |a| >= |b|, a != 0
		{
			r = -a;
			phi = (Pi / 4.0f) * (4.0f + (b / a));
		}
		else                        // region 4, |b| >= |a|, but a==0 and b==0 could occur.
		{
			r = -b;
			if (abs(b) > 0.0f)
				phi = (Pi / 4.0f) * (6.0f - (a / b));
			else
				phi = 0;
		}
	}

	const float N = numSides;
	float polyModifier = cos(Pi / N) / cos(phi - (Pi2 / N) * floor((N * phi + Pi) / Pi2));
	r *= lerp(1.0f, polyModifier, polygonAmount);

	float2 result;
	result.x = r * cos(phi);
	result.y = r * sin(phi);

	return result;
}

float Weigh(float coc, float radius)
{
	//return coc >= radius;
	return saturate((coc - radius + 2) / 2);
}

[numthreads(16, 16, 1)]
void main_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	source.GetDimensions(screen_size.x, screen_size.y);
	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y);
	float2 texelSize = 1.0f / screen_size;

	float2 uv = screen_coord / screen_size;

	const uint NumSamples = NumDOFSamples * NumDOFSamples;

	const float MaxKernelSize = maxBokehSize * 0.5f;

	float3 fgColor = float3(0, 0, 0);
	float fgWeight = 0.f;

	float3 bgColor = float3(0, 0, 0);
	float bgWeight = 0.f;

	float curSamp = GetDownSampledCoC(uv, texelSize);
	if (enable_dof > 0)
	{
		for (int i = 0; i < NumSamples; i++)
		{
			float lensX = saturate((i % NumDOFSamples) / max(NumDOFSamples - 1.0f, 1.0f));
			float lensY = saturate((i / NumDOFSamples) / max(NumDOFSamples - 1.0f, 1.0f));

			float2 kernelOffset = SquareToConcentricDiskMapping(lensX, lensY, float(num_blades), bokeh_poly_amount);

			float2 o = kernelOffset * MaxKernelSize;
			float radius = length(o);

			o *= texelSize;

			float2 finalUV = uv + o;

			float3 s = source.SampleLevel(s0, finalUV, 0).rgb;
			float coc = cocbuffer.SampleLevel(s1, finalUV, 0);
			//float coc = GetDownSampledCoC(finalUV, texelSize);
		
			float bgw = Weigh(max(0, coc * MaxKernelSize), radius);
			bgColor += s.rgb * bgw;
			bgWeight += bgw;

			float fgw = Weigh(-coc * MaxKernelSize, radius);
			fgColor += s.rgb * fgw;
			fgWeight += fgw;

		}
	}

	bgColor.rgb *= 1 / (bgWeight + (bgWeight == 0));
	fgColor.rgb *= 1 / (fgWeight + (fgWeight == 0));

	//boost foreground weight to reduce artefacts
	float bgfg = min(1, fgWeight * Pi / NumSamples);

	float3 finalColor = lerp(bgColor.rgb, fgColor.rgb, bgfg);

	output[int2(dispatch_thread_id.xy)] = float4(finalColor, bgfg);
}

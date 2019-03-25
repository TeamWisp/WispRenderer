#include "dof_properties.hlsl"
#include "dof_util.hlsl"

Texture2D source : register(t0);
Texture2D coc_buffer :register(t1);
RWTexture2D<float4> output : register(u0);
SamplerState s0 : register(s0);
SamplerState s1 : register(s1);

static const float PI = 3.141592654f;
static const float PI2 = 6.283185308f;

cbuffer CameraProperties : register(b0)
{
	float f_number;
	float bokeh_poly_amount;
	uint num_blades;
	int enable_dof;
};

float GetDownSampledCoC(float2 uv, float2 texel_size)
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
	source.GetDimensions(screen_size.x, screen_size.y);
	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y);
	float2 texel_size = 1.0f / screen_size;

	float2 uv = (screen_coord + 0.5f) / screen_size;

	const uint NUMSAMPLES = NUMDOFSAMPLES * NUMDOFSAMPLES;

	const float MAXKERNELSIZE = MAXBOKEHSIZE * 0.5f;

	float3 fgcolor = float3(0, 0, 0);
	float fgweight = 0.f;

	float3 bgcolor = float3(0, 0, 0);
	float bgweight = 0.f;

	float cur_samp = GetDownSampledCoC(uv, texel_size);
	//float cur_samp = coc_buffer.SampleLevel(s1, uv, 0);
	float kernel_radius = MAXKERNELSIZE * cur_samp;
	if (enable_dof > 0)
	{
		for (int i = 0; i < NUMSAMPLES; i++)
		{
			float lensX = saturate((i % NUMDOFSAMPLES) / max(NUMDOFSAMPLES - 1.0f, 1.0f));
			float lensY = saturate((i / NUMDOFSAMPLES) / max(NUMDOFSAMPLES - 1.0f, 1.0f));

			float2 kernel_offset = SquareToConcentricDiskMapping(lensX, lensY, float(num_blades), bokeh_poly_amount);

			float2 o = kernel_offset * kernel_radius;
			float radius = length(o);

			o *= texel_size;

			float2 final_uv = uv + o;

			float3 s = source.SampleLevel(s0, final_uv, 0).rgb;
			float coc = coc_buffer.SampleLevel(s1, final_uv , 0);
			//float coc = GetDownSampledCoC(final_uv, texel_size);
		
			float bgw = WeighCoC(max(0, coc * MAXBOKEHSIZE), radius);
			bgcolor += s.rgb * bgw;
			bgweight += bgw;

			float fgw = WeighCoC(-coc * MAXBOKEHSIZE, radius);
			fgcolor += s.rgb * fgw;
			fgweight += fgw;

		}
	}

	bgcolor.rgb *= 1 / (bgweight + (bgweight == 0));
	fgcolor.rgb *= 1 / (fgweight + (fgweight == 0));

	//boost foreground weight to reduce artefacts
	float bgfg = min(1, fgweight * PI / 2 / NUMSAMPLES);

	float3 finalcolor = lerp(bgcolor.rgb, fgcolor.rgb, bgfg);

	output[int2(dispatch_thread_id.xy)] = float4(finalcolor, bgfg);
}

#ifndef __PP_BLOOM_BLUR_VERTICAL_HLSL__
#define __PP_BLOOM_BLUR_VERTICAL_HLSL__

#include "pp_dof_util.hlsl"
#include "pp_bloom_util.hlsl"

Texture2D source : register(t0);
Texture2D source_qes : register(t1);
RWTexture2D<float4> output : register(u0);
RWTexture2D<float4> output_qes : register(u1);
SamplerState s0 : register(s0);

cbuffer BloomDirection : register(b0)
{
	float2 blur_direction;
	float _pad;
	float sigma;
};


float4 GetBlurFactor(float2 screen_coord, float res_scale)
{
	float2 screen_size = float2(0.f, 0.f);
	output.GetDimensions(screen_size.x, screen_size.y);

	float2 blur_dir = float2(0.0f, 1.0f);
	float r_sigma = 3.0f;
	float4 color = 0;
	float weightSum = 0.0f;
	for (int i = -7; i < 7; i++)
	{
		float weight = CalcGaussianWeight(i, r_sigma);
		weightSum += weight;
		float2 o_uv = saturate((screen_coord + (blur_dir * i)) / (screen_size / res_scale));
		float4 s = source.SampleLevel(s0, o_uv, 0);
		color += s * weight;
	}

	return color;
}

float4 GetBlurFactorQES(float2 screen_coord, float res_scale)
{
	float2 screen_size = float2(0.f, 0.f);
	output.GetDimensions(screen_size.x, screen_size.y);

	float2 blur_dir = float2(0.0f, 1.0f);
	float r_sigma = 3.0f;
	float4 color = 0;
	float weightSum = 0.0f;
	for (int i = -7; i < 7; i++)
	{
		float weight = CalcGaussianWeight(i, r_sigma);
		weightSum += weight;
		//float2 o_uv = saturate((screen_coord + (blur_dir * i)) / (screen_size / res_scale));
		//float2 o_uv = saturate((screen_coord + (blur_dir * i) / screen_size)) + (1 - 1 / res_scale);
		//float4 s = source_qes.SampleLevel(s0, o_uv, 0);
		float4 s = source_qes[screen_coord + blur_dir * i].rgba;
		color += s * weight;
	}

	return color;
}

[numthreads(16, 16, 1)]
void main_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	output.GetDimensions(screen_size.x, screen_size.y);

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y) + 0.5f;

	float4 color = 0;

	if (screen_coord.x > screen_size.x)
	{
		if (screen_coord.x > (screen_size.x * 1.75f))
		{
			color = GetBlurFactorQES(screen_coord - screen_size, 16.0f);
			
		}
		else if (screen_coord.x > (screen_size.x * 1.5f))
		{
			color = GetBlurFactorQES(screen_coord - screen_size, 8.0f);
			
		}
		else
		{
			color = GetBlurFactorQES(screen_coord - screen_size, 2.0f);
		}
		output_qes[screen_coord - int2(screen_size)] = color;
	}
	else
	{
		color = GetBlurFactor(screen_coord, 1.0f);
		output[screen_coord] = color;
	}
}

#endif //__PP_BLOOM_BLUR_VERTICAL_HLSL__
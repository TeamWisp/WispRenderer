#include "fullscreen_quad.hlsl"
#include "hdr_util.hlsl"

Texture2D source : register(t0);
Texture2D gbuffer_depth : register(t1);
RWTexture2D<float4> output : register(u0);
SamplerState s0 : register(s0);

cbuffer CameraProperties : register(b0)
{
	float4x4 view;
	float4x4 projection;
	float4x4 inv_projection;
	float4x4 inv_view;
};

static uint min_depth = 0xFFFFFFFF;
static uint max_depth = 0x0;

static const float focalLength = 2.0f;

static const float Pi = 3.141592654f;
static const float Pi2 = 6.283185307f;


float GetBlurFactor(float fDepthVS, float focusDist)
{
	// smoothstep 0, focalwidth, abs ( focaldistance  (depth * farclip)
	return smoothstep(0, 25.0f, abs(focusDist - (fDepthVS * focalLength)));
}

float GetCoC(float fDepthVS, float focusDist)
{
	float coc = (fDepthVS - focusDist) / focalLength;
	coc = clamp(coc, -1.f, 1.f);
	return coc;
}

float GetAutoFocusDepth(float2 screen_dimensions)
{
	float depth_focus = gbuffer_depth[float2(0.5f * screen_dimensions.x, 0.5f * screen_dimensions.y)].r;

	depth_focus += gbuffer_depth[float2(0.52f * screen_dimensions.x, 0.52f * screen_dimensions.y)].r;
	depth_focus += gbuffer_depth[float2(0.48f * screen_dimensions.x, 0.52f * screen_dimensions.y)].r;
	depth_focus += gbuffer_depth[float2(0.52f * screen_dimensions.x, 0.48f * screen_dimensions.y)].r;
	depth_focus += gbuffer_depth[float2(0.48f * screen_dimensions.x, 0.48f * screen_dimensions.y)].r;

	depth_focus += gbuffer_depth[float2(0.54f * screen_dimensions.x, 0.54f * screen_dimensions.y)].r;
	depth_focus += gbuffer_depth[float2(0.46f * screen_dimensions.x, 0.54f * screen_dimensions.y)].r;
	depth_focus += gbuffer_depth[float2(0.54f * screen_dimensions.x, 0.46f * screen_dimensions.y)].r;
	depth_focus += gbuffer_depth[float2(0.46f * screen_dimensions.x, 0.46f * screen_dimensions.y)].r;

	depth_focus += gbuffer_depth[float2(0.505f * screen_dimensions.x, 0.505f * screen_dimensions.y)].r;
	depth_focus += gbuffer_depth[float2(0.495f * screen_dimensions.x, 0.505f * screen_dimensions.y)].r;
	depth_focus += gbuffer_depth[float2(0.505f * screen_dimensions.x, 0.495f * screen_dimensions.y)].r;
	depth_focus += gbuffer_depth[float2(0.495f * screen_dimensions.x, 0.495f * screen_dimensions.y)].r;

	depth_focus += gbuffer_depth[float2(0.55f * screen_dimensions.x, 0.55f * screen_dimensions.y)].r;
	depth_focus += gbuffer_depth[float2(0.45f * screen_dimensions.x, 0.55f * screen_dimensions.y)].r;
	depth_focus += gbuffer_depth[float2(0.55f * screen_dimensions.x, 0.45f * screen_dimensions.y)].r;
	depth_focus += gbuffer_depth[float2(0.45f * screen_dimensions.x, 0.45f * screen_dimensions.y)].r;

	depth_focus = (depth_focus / 17.0f);

	return depth_focus;
}

float2 SquareToCentricDiskMapping(float x, float y)
{
	float polygonAmount = 22.3f;
	
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

	const float N = 5;
	float polyModifier = cos(Pi / N) / cos(phi - (Pi2 / N) * floor((N * phi + Pi) / Pi2));
	r *= lerp(1.0f, polyModifier, polygonAmount);

	float2 result;
	result.x = r * cos(phi);
	result.y = r * sin(phi);

	return result;
}


//static float2 kernel[12];

[numthreads(16, 16, 1)]
void main_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	source.GetDimensions(screen_size.x, screen_size.y);

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y);

	// Scale tap offsets based on render target size
	float dx = 1000.0f / screen_size.x ;
	float dy = 1000.0f / screen_size.y ;

	//float2 kernel[12] = { float2(-0.326212f * dx , -0.40581f * dy) ,
	//float2(-0.840144f * dx  , -0.07358f * dy),
	//float2(-0.840144f * dx  , 0.457137f * dy),
	//float2(-0.203345f * dx  , 0.620716f * dy),
	//float2(0.96234f * dx  , -0.194983f * dy),
	//float2(0.473434f * dx  , -0.480026f * dy),
	//float2(0.519456f * dx  , 0.767022f * dy),
	//float2(0.185461f * dx  , -0.893124f * dy),
	//float2(0.507431f * dx  , 0.064425f * dy),
	//float2(0.89642f * dx  , 0.412458f * dy),
	//float2(-0.32194f * dx  , -0.932615f * dy),
	//float2(-0.791559f * dx  , -0.59771f * dy) };

	const float depth_f = gbuffer_depth[screen_coord].r * focalLength;
	float depth_focus = GetAutoFocusDepth(screen_size);

	// Start with center sample color
	float3 vColorSum = source[screen_coord].xyz;;
	float fTotalContribution = 1.0f;

	// Depth and blurriness values for center sample
	float fCenterDepth = depth_focus * focalLength;
	float fCenterBlur = GetBlurFactor(depth_f, fCenterDepth);

	//const float ShapeCurve = 2.0f;
	//const float ShapeCurveAmt = 0.0f;

	//const float MaxKernelSize = 5.f * 0.5f;

	//float kernelRadius = MaxKernelSize * depth_focus;

	//[branch]
	//if (kernelRadius > 0.f)
	//{
	//	[unroll]
	//	for (uint i = 0; i < 49; ++i)
	//	{
	//		float lensX = saturate((i % 7) / max(7 - 1.0f, 1.0f));
	//		float lensY = saturate((i / 7) / max(7 - 1.0f, 1.0f));
	//		float2 kernelOffset = SquareToCentricDiskMapping(lensX, lensY);

	//		float2 vTapCoord = screen_coord + kernelOffset * kernelRadius;
	//		float3 vTapColor = source[vTapCoord];
	//		float fTapDepth = gbuffer_depth[vTapCoord].x;

	//		float fTapBlur = GetBlurFactor(fTapDepth, fCenterDepth);
	//		
	//		float fTapContribution = (fTapDepth > fCenterDepth) ? 1.0f : fTapBlur;

	//		vColorSum += vTapColor * fTapContribution;
	//		fTotalContribution += fTapContribution;

	//	}
	//}


	//if (fCenterBlur > 0)
	//{
	//	// Compute CoC size based on blurriness
	//	float fSizeCoC = fCenterBlur * 15.0f;

	//	// Run through all filter taps
	//	[unroll]
	//	for (int i = 0; i < 12; i++)
	//	{
	//		// Compute sample coordinates
	//		float2 vTapCoord = screen_coord + kernel[i] * fSizeCoC;

	//		// Fetch filter tap sample
	//		float3 vTapColor = source[vTapCoord].xyz;
	//		float fTapDepth = gbuffer_depth[vTapCoord].x;
	//		float fTapBlur = GetBlurFactor(fTapDepth, fCenterDepth);

	//		// Compute tap contribution based on depth and blurriness
	//		float fTapContribution = (fTapDepth > fCenterDepth) ? 1.0f : fTapBlur;

	//		// Accumulate color and sample contribution
	//		vColorSum += vTapColor * fTapContribution;
	//		fTotalContribution += fTapContribution;
	//	}
	//}

	static const int kernelSampleCount = 22;
	static const float2 kernel[kernelSampleCount] = {
		float2(0, 0),
		float2(0.53333336, 0),
		float2(0.3325279, 0.4169768),
		float2(-0.11867785, 0.5199616),
		float2(-0.48051673, 0.2314047),
		float2(-0.48051673, -0.23140468),
		float2(-0.11867763, -0.51996166),
		float2(0.33252785, -0.4169769),
		float2(1, 0),
		float2(0.90096885, 0.43388376),
		float2(0.6234898, 0.7818315),
		float2(0.22252098, 0.9749279),
		float2(-0.22252095, 0.9749279),
		float2(-0.62349, 0.7818314),
		float2(-0.90096885, 0.43388382),
		float2(-1, 0),
		float2(-0.90096885, -0.43388376),
		float2(-0.6234896, -0.7818316),
		float2(-0.22252055, -0.974928),
		float2(0.2225215, -0.9749278),
		float2(0.6234897, -0.7818316),
		float2(0.90096885, -0.43388376),
	};

	float2 TexelSize = (1.f / screen_size);

	if (fCenterBlur > 0)
	{
		// Compute CoC size based on blurriness
		float fSizeCoC = 8.0f;

		// Run through all filter taps
		[unroll]
		for (int i = 0; i < kernelSampleCount; i++)
		{
				float2 o = kernel[i] * fSizeCoC;
				// Compute sample coordinates
				float2 vTapCoord = screen_coord + o;

				// Fetch filter tap sample
				float3 vTapColor = source[vTapCoord].xyz;

				//float fTapDepth = gbuffer_depth[vTapCoord].x;
				//float fTapBlur = GetBlurFactor(fTapDepth, fCenterDepth);

				// Compute tap contribution based on depth and blurriness
				//float fTapContribution = (fTapDepth > fCenterDepth) ? 1.0f : fTapBlur;

				// Accumulate color and sample contribution

				vColorSum += vTapColor;
				
		}
	}

	// Normalize color sum
	float3 vFinalColor = vColorSum / kernelSampleCount;

	//Do shading
	output[int2(dispatch_thread_id.xy)] = float4(vFinalColor, 1.f);
}

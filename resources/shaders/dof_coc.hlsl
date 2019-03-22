#include "dof_properties.hlsl"

Texture2D gbuffer_depth : register(t0);
RWTexture2D<float4> output : register(u0);
SamplerState s0 : register(s0);

static const float fNear = 0.1f;
static const float fFar = 1000.f;

cbuffer CameraProperties : register(b0)
{
	float4x4 projection;
	float focal_length;
	float f_number;
	float film_size;
	float focus_dist;
	int enable_dof;
};

float GetLinearDepth(float depth)
{
	float z = (2 * fNear) / (fFar + fNear - (depth * (fFar - fNear)));
	return z;
}

float GetCoC(float linearDepth, float focusDist)
{
	float2 screen_size = float2(0.f, 0.f);
	output.GetDimensions(screen_size.x, screen_size.y);

	float fStop = focal_length / f_number * 0.5f;
	////// Compute CoC in meters
	float coc = -fStop * (focal_length * (focusDist - linearDepth)) / (linearDepth * (focusDist - focal_length));

	//// Convert to pixels
	coc = (coc / film_size) * screen_size.x;

	coc = clamp(coc / maxBokehSize, -1.f, 1.f);
	return coc * enable_dof;
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

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y);

	float2 uv = screen_coord / screen_size;
	
	float sampleDepth = gbuffer_depth[screen_coord].r;
	float focusDepth = GetAutoFocusDepth(screen_size);

	sampleDepth = GetLinearDepth(sampleDepth) * fFar;

	float coc = GetCoC(sampleDepth, focus_dist);

	if (focus_dist < 1)
	{
		focusDepth = GetLinearDepth(focusDepth) * fFar;
		coc = GetCoC(sampleDepth, focusDepth);
	}
	
	output[int2(dispatch_thread_id.xy)] = coc;
}

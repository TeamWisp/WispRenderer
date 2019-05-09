#include "dof_properties.hlsl"
#include "dof_util.hlsl"

Texture2D gbuffer_depth : register(t0);
RWTexture2D<float4> output : register(u0);
SamplerState s0 : register(s0);

cbuffer CameraProperties : register(b0)
{
	float4x4 projection;
	float focal_length;
	float f_number;
	float film_size;
	float focus_dist;
	int enable_dof;
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

	coc = clamp(coc / MAXBOKEHSIZE, -1.f, 1.f);
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

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y) + 0.5f;

	float2 uv = screen_coord / screen_size;
	
	float sample_depth = gbuffer_depth[screen_coord].r;
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
	output[int2(dispatch_thread_id.xy)] = coc;
}

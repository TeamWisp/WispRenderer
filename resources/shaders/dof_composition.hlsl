#include "fullscreen_quad.hlsl"
#include "dof_properties.hlsl"

Texture2D source : register(t0);
RWTexture2D<float4> output : register(u0);
Texture2D bokeh : register(t1);
Texture2D cocbuffer : register(t2);
SamplerState s0 : register(s0);
SamplerState s1 : register(s1);


static uint min_depth = 0xFFFFFFFF;
static uint max_depth = 0x0;


float GetDownSampledCoC(float2 uv , float2 texelSize)
{
	float4 offset = texelSize.xyxy * float2(-0.5f, 0.5f).xxyy;
	float coc0 = cocbuffer.SampleLevel(s0, uv + offset.xy, 0);
	float coc1 = cocbuffer.SampleLevel(s0, uv + offset.zy, 0);
	float coc2 = cocbuffer.SampleLevel(s0, uv + offset.xw, 0);
	float coc3 = cocbuffer.SampleLevel(s0, uv + offset.zw, 0);

	float cocMin = min(min(min(coc0, coc1), coc2), coc3);
	float cocMax = max(max(max(coc0, coc1), coc2), coc3);

	float coc = cocMax >= -cocMin ? cocMax : cocMin;

	return coc;
}

[numthreads(16, 16, 1)]
void main_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	output.GetDimensions(screen_size.x, screen_size.y);

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y);

	float2 texelSize = 1.0 / screen_size;
	float2 Uuv = (screen_coord + 0.5f) / screen_size;
	float2 uv = screen_coord / screen_size;
	// Start with center sample color
	float3 vColorSum = source[screen_coord].xyz;;

	uv = clamp(uv, 0.002, 0.998);
	Uuv = clamp(Uuv, 0.002, 0.998);

	// Normalize color sum
	float3 Bokeh = bokeh.SampleLevel(s1, uv, 0).rgb; //vColorSum / kernelSampleCount;
	float bgfg = bokeh.SampleLevel(s0, uv, 0).a;

	float3 Original = source.SampleLevel(s1, uv, 0).rgb;

	float coc = cocbuffer.SampleLevel(s0, uv, 0).a * maxBokehSize ;

	float dofStrength = smoothstep(0.0f, 1.0f, abs(coc));

	float3 color = float3(0, 0, 0);

	color = lerp(Original, Bokeh, smoothstep(0.0f,1.0f,(dofStrength + bgfg) - (dofStrength * bgfg)));

	output[int2(dispatch_thread_id.xy)] = float4(color, 1.f);
}

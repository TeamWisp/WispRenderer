#include "dof_properties.hlsl"

Texture2D source : register(t0);
RWTexture2D<float4> output : register(u0);
Texture2D bokeh_buffer : register(t1);
Texture2D coc_buffer : register(t2);
SamplerState s0 : register(s0);
SamplerState s1 : register(s1);

float GetDownSampledCoC(float2 uv , float2 texel_size)
{
	float4 offset = texel_size.xyxy * float2(-0.5f, 0.5f).xxyy;
	float coc0 = coc_buffer.SampleLevel(s0, uv + offset.xy, 0);
	float coc1 = coc_buffer.SampleLevel(s0, uv + offset.zy, 0);
	float coc2 = coc_buffer.SampleLevel(s0, uv + offset.xw, 0);
	float coc3 = coc_buffer.SampleLevel(s0, uv + offset.zw, 0);

	float coc_min = min(min(min(coc0, coc1), coc2), coc3);
	float coc_max = max(max(max(coc0, coc1), coc2), coc3);

	float coc = coc_max >= -coc_min ? coc_max : coc_min;
	return coc;
}

[numthreads(16, 16, 1)]
void main_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	output.GetDimensions(screen_size.x, screen_size.y);

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y);

	float2 texel_size = 1.0 / screen_size;
	//float2 Uuv = (screen_coord + 0.5f) / screen_size;
	float2 uv = screen_coord / screen_size;

	float3 bokeh = bokeh_buffer.SampleLevel(s1, uv, 0).rgb;
	float bgfg = bokeh_buffer.SampleLevel(s0, uv, 0).a ;

	float3 original = source.SampleLevel(s1, uv, 0).rgb;

	float coc = GetDownSampledCoC(uv, texel_size);
	//float coc = coc_buffer.SampleLevel(s0, uv, 0);
	float dofstrength = smoothstep(0.0f, 1.0f, abs(coc));

	float3 color = lerp(original, bokeh, dofstrength + bgfg - dofstrength * bgfg);

	output[int2(dispatch_thread_id.xy)] = float4(color, 1.f);
}

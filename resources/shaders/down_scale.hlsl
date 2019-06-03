#include "dof_util.hlsl"

Texture2D source : register(t0);
RWTexture2D<float4> output_near : register(u0);
RWTexture2D<float4> output_far : register(u1);
Texture2D cocbuffer : register(t1);
SamplerState s0 : register(s0);
SamplerState s1 : register(s1);

float GetDownSampledCoC(float2 uv, float2 texelSize)
{
	float4 offset = texelSize.xyxy * float2(-0.5f, 0.5f).xxyy;

	float coc0 = cocbuffer.SampleLevel(s1, uv + offset.xy, 0).r;
	float coc1 = cocbuffer.SampleLevel(s1, uv + offset.zy, 0).r;
	float coc2 = cocbuffer.SampleLevel(s1, uv + offset.xw, 0).r;
	float coc3 = cocbuffer.SampleLevel(s1, uv + offset.zw, 0).r;

	float cocMin = min(min(min(coc0, coc1), coc2), coc3);
	float cocMax = max(max(max(coc0, coc1), coc2), coc3);

	float coc = cocMax >= -cocMin ? cocMax : cocMin;

	//float coc = (coc0 + coc1 + coc2 + coc3) * 0.25f;
	return coc;
}

[numthreads(16, 16, 1)]
void main_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	output_far.GetDimensions(screen_size.x, screen_size.y);

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y) + 0.5f;

	float2 texel_size = 1.0f / screen_size;
	float4 offset = texel_size.xyxy * float2(-0.5, 0.5).xxyy;

	float2 uv = screen_coord / screen_size;

	float3 source0 = source.SampleLevel(s0, uv + offset.xy, 0).rgb;
	float3 source1 = source.SampleLevel(s0, uv + offset.zy, 0).rgb;
	float3 source2 = source.SampleLevel(s0, uv + offset.xw, 0).rgb;
	float3 source3 = source.SampleLevel(s0, uv + offset.zw, 0).rgb;

	float3 finalcolor = (source0 + source1 + source2 + source3) * 0.25f;

	float coc = GetDownSampledCoC(uv, texel_size);

	float4 out_near = max(0,float4(finalcolor, 1.0f) * max(-coc, 0.0f));
	out_near.rgb = finalcolor;

	float4 out_far = max(0,float4(finalcolor, 1.0f) * max(coc, 0.0f));

	output_near[int2(dispatch_thread_id.xy)] = out_near;
	output_far[int2(dispatch_thread_id.xy)] = out_far;
}

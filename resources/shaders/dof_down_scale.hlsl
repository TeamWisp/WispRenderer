#include "hdr_util.hlsl"

Texture2D source : register(t0);
RWTexture2D<float4> output : register(u0);
Texture2D cocbuffer : register(t1);
SamplerState s0 : register(s0);
SamplerState s1 : register(s1);

float GetDownSampledCoC(float2 uv, float2 screen_coord, float2 texelSize)
{
	float4 offset = texelSize.xyxy * float2(-0.5f, 0.5f).xxyy;

	float coc0 = cocbuffer.SampleLevel(s1, uv + offset.xy, 0);
	float coc1 = cocbuffer.SampleLevel(s1, uv + offset.zy, 0);
	float coc2 = cocbuffer.SampleLevel(s1, uv + offset.xw, 0);
	float coc3 = cocbuffer.SampleLevel(s1, uv + offset.zw, 0);

	float cocMin = min(min(min(coc0, coc1), coc2), coc3);
	float cocMax = max(max(max(coc0, coc1), coc2), coc3);

	float coc = cocMax >= -cocMin ? cocMax : cocMin;

	return coc;
}

float Weigh(float3 color)
{
	return 1 / (1 + max(max(color.r, color.g), color.b));
}

[numthreads(16, 16, 1)]
void main_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	output.GetDimensions(screen_size.x, screen_size.y);

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y);

	float2 texelSize = 1.0f / screen_size;
	float4 offset = texelSize.xyxy * float2(-0.5, 0.5).xxyy;

	float2 uv = (screen_coord + 0.5f) / screen_size;

	float3 source0 = source.SampleLevel(s0, uv + offset.xy, 0).rgb;
	float3 source1 = source.SampleLevel(s0, uv + offset.zy, 0).rgb;
	float3 source2 = source.SampleLevel(s0, uv + offset.xw, 0).rgb;
	float3 source3 = source.SampleLevel(s0, uv + offset.zw, 0).rgb;

	float w0 = Weigh(source0);
	float w1 = Weigh(source1);
	float w2 = Weigh(source2);
	float w3 = Weigh(source3);

	float3 vFinalColor = source0 * w0 + source1 * w1 + source2 * w2 + source3 * w3;
	vFinalColor /= max(w0 + w1 + w2 + w3, 0.00001f);

	float CoC = GetDownSampledCoC(uv, screen_coord, texelSize);

	output[int2(dispatch_thread_id.xy)] = float4(vFinalColor, CoC);
}

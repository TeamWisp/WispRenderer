Texture2D source : register(t0);
RWTexture2D<float4> output : register(u0);
SamplerState s0 : register(s0);
SamplerState s1 : register(s1);

[numthreads(16, 16, 1)]
void main_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	output.GetDimensions(screen_size.x, screen_size.y);

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y);

	float2 texelSize = 1.0f / screen_size;
	float4 offset = texelSize.xyxy * float2(-0.5, 0.5).xxyy;

	float2 uv = (screen_coord + 0.5f) / screen_size;

	float bgfg = source.SampleLevel(s1, uv, 0).a;

	float3 vFinalColor = 
		source.SampleLevel(s0, uv + offset.xy, 0).rgb +
		source.SampleLevel(s0, uv + offset.zy, 0).rgb +
		source.SampleLevel(s0, uv + offset.xw, 0).rgb +
		source.SampleLevel(s0, uv + offset.zw, 0).rgb;
	
	vFinalColor *= 0.25f;

	vFinalColor = source.SampleLevel(s0, uv, 0).rgb;
	output[int2(dispatch_thread_id.xy)] = float4(vFinalColor, bgfg);
}

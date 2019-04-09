Texture2D source_near : register(t0);
Texture2D source_far : register(t1);
RWTexture2D<float4> output_near : register(u0);
RWTexture2D<float4> output_far : register(u1);
SamplerState s0 : register(s0);
SamplerState s1 : register(s1);

[numthreads(16, 16, 1)]
void main_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	output_near.GetDimensions(screen_size.x, screen_size.y);

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y);

	float2 texel_size = 1.0f / screen_size;
	float4 offset = texel_size.xyxy * float2(-0.5, 0.5).xxyy;

	float2 uv = (screen_coord + 0.5f) / screen_size;

	//float bgfg = source.SampleLevel(s1, uv, 0).a;
	float nearcoc = source_near.SampleLevel(s1, uv, 0).a;
	float farcoc = source_far.SampleLevel(s1, uv, 0).a;

	float3 near_color = 
		source_near.SampleLevel(s0, uv + offset.xy, 0).rgb +
		source_near.SampleLevel(s0, uv + offset.zy, 0).rgb +
		source_near.SampleLevel(s0, uv + offset.xw, 0).rgb +
		source_near.SampleLevel(s0, uv + offset.zw, 0).rgb;
	
	near_color *= 0.25f;

	float3 far_color =
		source_far.SampleLevel(s0, uv + offset.xy, 0).rgb +
		source_far.SampleLevel(s0, uv + offset.zy, 0).rgb +
		source_far.SampleLevel(s0, uv + offset.xw, 0).rgb +
		source_far.SampleLevel(s0, uv + offset.zw, 0).rgb;

	far_color *= 0.25f;

	output_near[int2(dispatch_thread_id.xy)] = float4(near_color, nearcoc);
	output_far[int2(dispatch_thread_id.xy)] = float4(far_color, farcoc);
}

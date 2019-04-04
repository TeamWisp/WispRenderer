Texture2D<float4> noisy_data : register(t0);
Texture2D<float4> accum : register(t1);
RWTexture2D<float4> output   : register(u0);
RWTexture2D<float4> variance : register(u1);
SamplerState point_sampler   : register(s0);

[numthreads(16,16,1)]
void temporal_accumulator_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	output.GetDimensions(screen_size.x, screen_size.y);

	float2 uv = float2(dispatch_thread_id.x / screen_size.x, dispatch_thread_id.y / screen_size.y);

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y);

	float4 accum_data = accum[screen_coord];
	float4 noise = noisy_data[screen_coord];

	float min_val = 1.0;
	float max_val = 0.0;

	for(int i = -3; i<4; ++i)
	{
		for(int j = -3; j<4; ++j)
		{
			float4 noise_sample = noisy_data[screen_coord + int2(i, j)];
			min_val = min(min_val, noise_sample.x);
			max_val = max(max_val, noise_sample.x);
		}
	}

	if(accum_data.w==0.f)
	{
		accum_data = float4(1, 1, 1, 0);
	}

	accum_data.w = clamp(accum_data.w, 1.0, 4.0);

	accum_data = float4(
		clamp(accum_data.x, min_val, max_val), 
		clamp(accum_data.y, min_val, max_val), 
		clamp(accum_data.z, min_val, max_val),
		lerp(accum_data.w, 1.0, accum_data.x<min_val || accum_data.x > max_val));

	accum_data = float4(accum_data.x*accum_data.w + noise.x, accum_data.y*accum_data.w + noise.y, accum_data.z*accum_data.w + noise.z, accum_data.w+1);
	accum_data = float4(accum_data.x/accum_data.w, accum_data.y/accum_data.w, accum_data.z/accum_data.w, accum_data.w);

	output[screen_coord] = 0.5;
	variance[screen_coord] = 0.5;
}
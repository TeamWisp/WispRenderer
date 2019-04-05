Texture2D<float4> noisy_data : register(t0);
Texture2D<float4> accum : register(t1);
RWTexture2D<float4> output   : register(u0);
RWTexture2D<float4> variance : register(u1);
SamplerState point_sampler   : register(s0);


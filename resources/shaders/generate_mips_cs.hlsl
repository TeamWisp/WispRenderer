Texture2D<float4> src_texture : register(t0);
RWTexture2D<float4> dst_texture : register(u0);
SamplerState bilinear_clamp : register(s0);

cbuffer CB : register(b0)
{
	float2 texel_size;   // 1.0 / destination dimension
}

[numthreads(8, 8, 1)]
void main(uint3 dt_id : SV_DispatchThreadID)
{
	//DTid is the thread ID * the values from numthreads above and in this case correspond to the pixels location in number of pixels.
	//As a result texcoords (in 0-1 range) will point at the center between the 4 pixels used for the mipmap.
	float2 tex_coords = texel_size * (dt_id.xy + 0.5f);

	//The samplers linear interpolation will mix the four pixel values to the new pixels color
	float4 color = src_texture.SampleLevel(bilinear_clamp, tex_coords, 0);

	//Write the final color into the destination texture.
	dst_texture[dt_id.xy] = color;
}
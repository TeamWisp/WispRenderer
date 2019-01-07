Texture2D<float4> src_texture : register(t0);
Texture2D<float4> dst_texture : register(t1);
SamplerState s0 : register(s0);

cbuffer CameraProperties : register(b0)
{
	float4x4 view;
	float4x4 inv_projection_view;
	float3 camera_position;
	float padding;

	float dep_light_radius;
	float dep_metal;
	float frame_idx;
	float intensity;
};

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
	float2 pos = input.pos;
	//float4 previous = src_texture.SampleLevel(s0, input.uv, 0);
	//float4 current = dst_texture.SampleLevel(s0, input.uv, 0);
	float4 current = src_texture[pos];
	float4 previous = dst_texture[pos];

	float accum_count = frame_idx;
	
	return current / accum_count;
	if (frame_idx > 0)
	{
		return (accum_count * previous + current) / (accum_count + 1);
	}
	return current;
}

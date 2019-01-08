Texture2D<float4> src_texture : register(t0);
SamplerState s0 : register(s0);

cbuffer CameraProperties : register(b0)
{
	float4x4 view;
	float4x4 inv_projection_view;
	float3 camera_position;
	float padding;

	float unused0;
	float unused1;
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
	float4 current = src_texture[pos];

	float accum_count = frame_idx;
	
	return current;
	return current / accum_count;
}

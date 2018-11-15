struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

cbuffer CameraProperties : register(b0)
{
	float4x4 view;
	float4x4 projection;
};

cbuffer ObjectProperties : register(b1)
{
	float4x4 model;
};

VS_OUTPUT main_vs(float3 pos : POSITION)
{
	VS_OUTPUT output;

	float4x4 vm = mul(view, model);
	float4x4 mvp = mul(projection, vm);
	
	output.pos =  mul(mvp, float4(pos, 1.0f));
	output.uv = 0.5 * (pos.xy + float2(1.0, 1.0));

	return output;
}

float4 main_ps(VS_OUTPUT input) : SV_TARGET
{
	return float4(1, 0, 0, 1);
}

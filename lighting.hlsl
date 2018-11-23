
struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

VS_OUTPUT main_vs(float2 pos : POSITION)
{
	VS_OUTPUT output;
	output.pos = float4(pos, 0, 1);
	output.uv = pos * 0.5 + 0.5;
	return output;
}

//Texture2D gdiffuse : register(t0);
//Texture2D gnormal : register(t1);

sampler gsampler : register(s0);

float4 main_ps(VS_OUTPUT input) : SV_TARGET
{
	float2 uv = input.uv;

	/*float3 diffuse = gdiffuse.Sample(gsampler, uv).rgb;
	float3 normal = gnormal.Sample(gsampler, uv).xyz;*/

	return float4(uv, 0, 1);
}
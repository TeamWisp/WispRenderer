
//48 KiB; 48 * 1024 / sizeof(MeshNode)
//48 * 1024 / (4 * 4 * 4) = 48 * 1024 / 64 = 48 * 16 = 768
#define MAX_INSTANCES 768

struct VS_INPUT
{
	float4 pos : POSITION;
	float4 normal : NORMAL;
};

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
};

cbuffer CameraProperties : register(b0)
{
	float4x4 view;
	float4x4 projection;
	float4x4 inv_projection;
};

struct ObjectData
{
	float4x4 model;
};

cbuffer ObjectProperties : register(b1)
{
	ObjectData instances[MAX_INSTANCES];
};

//Decode position from axis bounds
float3 decode_pos(float3 pos)
{
	return pos * 2 - 1;
}

//Decode uv from axis bounds
float2 decode_uv(float2 uv)
{
	return uv;
}

VS_OUTPUT main_vs(VS_INPUT input, uint instid : SV_InstanceId)
{
	VS_OUTPUT output;

	float3 pos = decode_pos(input.pos.xyz);
	float2 uv = decode_uv(float2(input.pos.w, input.normal.w));

	ObjectData inst = instances[instid];

	//TODO: Use precalculated MVP or at least VP
	float4x4 vm = mul(view, inst.model);
	float4x4 mvp = mul(projection, vm);
	
	output.pos = mul(mvp, float4(pos, 1.0f));
	output.uv = uv;
	output.normal = normalize(mul(vm, input.normal.xyz * 2 - 1)).xyz;

	return output;
}

struct PS_OUTPUT
{
	float4 albedo : SV_TARGET0;
	half2 normal : SV_TARGET1;
};

//Spheremap Transform normal compression encode (aras-p.info)
half2 encode_normal(float3 n)
{
	half p = sqrt(n.z * 8 + 8);
	return half2(n.xy / p + 0.5);
}

PS_OUTPUT main_ps(VS_OUTPUT input) : SV_TARGET
{
	PS_OUTPUT output;
	output.albedo = float4(input.uv, 0, 1);
	output.normal = encode_normal(input.normal);
	return output;
}

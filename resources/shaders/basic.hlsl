
//48 KiB; 48 * 1024 / sizeof(ObjectData)
//48 * 1024 / 144 = 341
#define MAX_INSTANCES 341

//48 KiB; 48 * 1024 / sizeof(ModelData)
//48 * 1024 / 48 = 1024
#define MAX_MODELS 1024

//pos.w, normal.w = vuv
//pos.xyz = vpos
//normal.xyz = vnormal
//tangent.xyz = vtangent
//tangent.w, bitangent.xy = vbitangent
struct VS_INPUT
{
	float4 pos : POSITION;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
	float2 bitangent : BITANGENT;
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
	float4x4 mvp;

	float4x4 vm;

	uint3 pad;
	uint model_id;
};

struct ModelData
{
	float3 pos_start;
	uint is_defined;

	float3 pos_length;
	float pad_1;

	float4 uv_start_length;
};

cbuffer ObjectProperties : register(b1)
{
	ObjectData instances[MAX_INSTANCES];
};

cbuffer MeshProperties : register(b2)
{
	ModelData models[MAX_MODELS];
};

//Decode position from axis bounds
float3 decode_pos(float3 pos, float3 pos_start, float3 pos_length)
{
	return pos_start + pos * pos_length;
}

//Decode uv from axis bounds
float2 decode_uv(float2 uv, float4 uv_start_length)
{
	return uv_start_length.xy + uv * uv_start_length.zw;
}

float3 decode_direction(float3 dir)
{
	return dir * 2 - 1;
}

VS_OUTPUT main_vs(VS_INPUT input, uint instid : SV_InstanceId)
{
	VS_OUTPUT output;

	ObjectData inst = instances[instid];
	ModelData mod = models[inst.model_id];

	float3 pos = decode_pos(input.pos.xyz, mod.pos_start, mod.pos_length);
	float2 uv = decode_uv(float2(input.pos.w, input.normal.w), mod.uv_start_length);
	float3 normal = decode_direction(input.normal.xyz);
	//float3 tangent = decode_direction(input.tangent.xyz);
	//float3 bitangent = decode_direction(float3(input.tangent.w, input.bitangent.xy));
	
	output.pos = mul(inst.mvp, float4(pos, 1.0f));
	output.uv = uv;
	output.normal = normalize(mul(inst.vm, normal)).xyz;

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

//48 KiB; 48 * 1024 / sizeof(MeshNode)
//48 * 1024 / (4 * 4 * 4) = 48 * 1024 / 64 = 48 * 16 = 768
#define MAX_INSTANCES 768

#include "material_util.hlsl"

struct VS_INPUT
{
	float3 pos : POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 bitangent : BITANGENT;
	float3 color : COLOR;
};

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float4 prev_pos : PREV_POSITION;
	float4 curr_pos : CURR_POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 bitangent : BITANGENT;
	float3 color : COLOR;
};

cbuffer CameraProperties : register(b0)
{
	float4x4 view;
	float4x4 projection;
	float4x4 inv_projection;
	float4x4 inv_view;
	float4x4 prev_projection;
	float4x4 prev_view;
	uint is_hybrid;
	uint is_path_tracer;
};

struct ObjectData
{
	float4x4 model;
	float4x4 prev_model;
};

cbuffer ObjectProperties : register(b1)
{
	ObjectData instances[MAX_INSTANCES];
};

VS_OUTPUT main_vs(VS_INPUT input, uint instid : SV_InstanceId)
{
	VS_OUTPUT output;

	float3 pos = input.pos;

	ObjectData inst = instances[instid];

	//TODO: Use precalculated MVP or at least VP
	float4x4 vm = mul(view, inst.model);
	float4x4 mvp = mul(projection, vm);

	float4x4 prev_mvp = mul(prev_projection, mul(prev_view, inst.prev_model));
	
	output.pos =  mul(mvp, float4(pos, 1.0f));
	output.curr_pos = output.pos;
	output.prev_pos = mul(prev_mvp, float4(pos, 1.0f));
	output.uv = float2(input.uv.x, 1.0f - input.uv.y);
	output.tangent = normalize(mul(inst.model, float4(input.tangent, 0))).xyz;
	output.bitangent = normalize(mul(inst.model, float4(input.bitangent, 0))).xyz;
	output.normal = normalize(mul(inst.model, float4(input.normal, 0))).xyz;
	output.color = input.color;

	return output;
}

struct PS_OUTPUT
{
	float4 albedo_roughness : SV_TARGET0;
	float4 normal_metallic : SV_TARGET1;
	float4 velocity : SV_TARGET2;
};

Texture2D material_albedo : register(t0);
Texture2D material_normal : register(t1);
Texture2D material_roughness : register(t2);
Texture2D material_metallic : register(t3);

SamplerState s0 : register(s0);

cbuffer MaterialProperties : register(b2)
{
	MaterialData data;
}


PS_OUTPUT main_ps(VS_OUTPUT input) : SV_TARGET
{
	PS_OUTPUT output;
	float3x3 tbn = {input.tangent, input.bitangent, input.normal};

	OutputMaterialData output_data = InterpretMaterialData(data,
		material_albedo,
		material_normal,
		material_roughness,
		material_metallic,
		s0,
		input.uv);

	float3 normal = normalize(mul(output_data.normal, tbn));

	output.albedo_roughness = float4(pow(output_data.albedo, 2.2f), output_data.roughness);
	output.normal_metallic = float4(normal, output_data.metallic);

	float2 curr_pos = float2(input.curr_pos.xy / input.curr_pos.w) * 0.5 + 0.5;
	float2 prev_pos = float2(input.prev_pos.xy / input.prev_pos.w) * 0.5 + 0.5;

	output.velocity = float4(curr_pos.x - prev_pos.x, - (curr_pos.y - prev_pos.y), 0.0, 1.0);

	return output;
}

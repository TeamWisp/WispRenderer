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
};

struct ObjectData
{
	float4x4 model;
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
	
	output.pos =  mul(mvp, float4(pos, 1.0f));
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
};

Texture2D material_albedo : register(t0);
Texture2D material_normal : register(t1);
Texture2D material_roughness : register(t2);
Texture2D material_metallic : register(t3);

SamplerState s0 : register(s0);

struct MaterialData
{
	float4 albedo_alpha;
	float4 metallic_roughness;
	uint flags;
	uint3 padding;
};

cbuffer MaterialProperties : register(b2)
{
	MaterialData data;
}


PS_OUTPUT main_ps(VS_OUTPUT input) : SV_TARGET
{
	PS_OUTPUT output;
	float3x3 tbn = {input.tangent, input.bitangent, input.normal};

	uint use_albedo_constant = data.flags & MATERIAL_USE_ALBEDO_CONSTANT;
	uint has_albedo_texture = data.flags & MATERIAL_HAS_ALBEDO_TEXTURE;
	uint use_roughness_constant = data.flags & MATERIAL_USE_ROUGHNESS_CONSTANT;
	uint has_roughness_texture = data.flags & MATERIAL_HAS_ROUGHNESS_TEXTURE;
	uint use_metallic_constant = data.flags & MATERIAL_USE_METALLIC_CONSTANT;
	uint has_metallic_texture = data.flags & MATERIAL_HAS_METALLIC_TEXTURE;
	uint use_normal_texture = data.flags & MATERIAL_HAS_NORMAL_TEXTURE;

	float4 albedo = lerp(material_albedo.Sample(s0, input.uv), float4(data.albedo_alpha.xyz, 1.0f), use_albedo_constant!=0 || has_albedo_texture==0);

//#define COMPRESSED_PBR
#ifdef COMPRESSED_PBR
	float4 roughness = lerp(material_metallic.SampleLevel(s0, input.uv, 0).y, data.metallic_roughness.w, use_roughness_constant!=0 || has_roughness_texture == 0);
	float4 metallic = lerp(material_metallic.SampleLevel(s0, input.uv, 0).z, length(data.metallic_roughness.xyz), use_metallic_constant != 0 || has_metallic_texture == 0);
#else
	float4 roughness = lerp(max(0.05f, material_roughness.Sample(s0, input.uv)), data.metallic_roughness.wwww, use_roughness_constant != 0 || has_roughness_texture == 0);
	float4 metallic = lerp(material_metallic.Sample(s0, input.uv), data.metallic_roughness.xyzx, use_metallic_constant != 0 || has_metallic_texture == 0);
#endif
	float3 tex_normal = lerp(material_normal.Sample(s0, input.uv).rgb * 2.0 - float3(1.0, 1.0, 1.0), float3(0.0, 0.0, 1.0), use_normal_texture == 0);
	float3 normal = normalize(mul(tex_normal, tbn));

	output.albedo_roughness = float4(albedo.xyz, roughness.r);
	output.normal_metallic = float4(normal, metallic.r);
	return output;
}

/*!
 * Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __DEFERRED_GEOMETRY_PASS_HLSL__
#define __DEFERRED_GEOMETRY_PASS_HLSL__

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
};

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	#ifdef IS_HYBRID
	float4 prev_pos : PREV_POSITION;
	float4 curr_pos : CURR_POSITION;
	float4 world_pos : WORLD_POSITION;
	#endif
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 bitangent : BITANGENT;
	#ifdef IS_HYBRID
	float3 obj_normal : OBJECT_NORMAL;
	float3 obj_tangent : OBJECT_TANGENT;
	float3 obj_bitangent : OBJECT_BITANGENT;
	#endif
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
	uint is_ao;
	uint has_shadows;

	float3 padding1;
	uint has_reflections;
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
	#ifdef IS_HYBRID
	output.curr_pos = output.pos;
	output.prev_pos = mul(prev_mvp, float4(pos, 1.0f));
	output.world_pos = mul(inst.model, float4(pos, 1.0f));
	#endif
	output.uv = float2(input.uv.x, 1.0f - input.uv.y);
	output.tangent = normalize(mul(inst.model, float4(input.tangent, 0))).xyz;
	output.bitangent = normalize(mul(inst.model, float4(input.bitangent, 0))).xyz;
	output.normal = normalize(mul(inst.model, float4(input.normal, 0))).xyz;
	#ifdef IS_HYBRID
	output.obj_normal = input.normal.xyz;
	output.obj_tangent = input.tangent.xyz;
	output.obj_bitangent = input.bitangent.xyz;
	#endif

	return output;
}

struct PS_OUTPUT
{
	float4 albedo_roughness : SV_TARGET0;
	float4 normal_metallic : SV_TARGET1;
	float4 emissive_ao : SV_TARGET2;
	#ifdef IS_HYBRID
	float4 velocity : SV_TARGET3;
	float4 depth : SV_TARGET4;
	float4 world_position : SV_TARGET5;
	#endif
};

Texture2D material_albedo : register(t0);
Texture2D material_normal : register(t1);
Texture2D material_roughness : register(t2);
Texture2D material_metallic : register(t3);
Texture2D material_ao : register(t4);
Texture2D material_emissive : register(t5);

SamplerState s0 : register(s0);

cbuffer MaterialProperties : register(b2)
{
	MaterialData data;
}

uint dirToOct(float3 normal)
{
	float2 p = normal.xy * (1.0 / dot(abs(normal), 1.0.xxx));
	float2 e = normal.z > 0.0 ? p : (1.0 - abs(p.yx)) * (step(0.0,p)*2.0-(float2)(1.0)); 
	return (asuint(f32tof16(e.y)) << 16) + (asuint(f32tof16(e.x)));
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
		material_emissive,
		material_ao,
		s0,
		input.uv);

	if (output_data.alpha <= 0.5f)
	{
		discard;
	}

	float3 normal = normalize(mul(output_data.normal, tbn));

	output.albedo_roughness = float4(output_data.albedo.xyz, output_data.roughness);
	output.normal_metallic = float4(normal, output_data.metallic);
	output.emissive_ao = float4(output_data.emissive, output_data.ao);

	#ifdef IS_HYBRID
	float3x3 obj_tbn = {input.obj_tangent, input.obj_bitangent, input.obj_normal};

	float3 obj_normal = normalize(mul(output_data.normal, obj_tbn));

	float2 curr_pos = float2(input.curr_pos.xy / input.curr_pos.w) * 0.5 + 0.5;
	float2 prev_pos = float2(input.prev_pos.xy / input.prev_pos.w) * 0.5 + 0.5;

	const float epsilon = 1e-5;

	float2 motion_vec = lerp(float2(curr_pos.x - prev_pos.x, -(curr_pos.y - prev_pos.y)), float2(0.0, 0.0), input.prev_pos.w < epsilon);

	output.velocity = float4(motion_vec.xy, length(fwidth(input.world_pos.xyz)), length(fwidth(normal)));

	float linear_z = input.pos.z * input.pos.w;
	float prev_z = input.prev_pos.z;
	float max_change_z = max(abs(ddx(linear_z)), abs(ddy(linear_z)));
	float compressed_obj_normal = asfloat(dirToOct(normalize(obj_normal)));
	output.depth = float4(linear_z, max_change_z, prev_z, compressed_obj_normal);

	output.world_position = input.world_pos;
	#endif

	return output;
}

#endif //__DEFERRED_GEOMETRY_PASS_HLSL__

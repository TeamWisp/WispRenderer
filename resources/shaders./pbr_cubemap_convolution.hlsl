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
	float3 local_pos : LOCPOS;
};

cbuffer PassIndex : register (b0)
{
	int idx;
}

cbuffer CameraProperties : register(b1)
{
	float4x4 projection;
	float4x4 view[6];
};

VS_OUTPUT main_vs(VS_INPUT input)
{
	VS_OUTPUT output;

	output.local_pos = input.pos.xyz;

	float4x4 vp = mul(projection, view[idx]);
	output.pos =  mul(vp, float4(output.local_pos, 1.0f));

	return output;
}

struct PS_OUTPUT
{
	float4 color;
};

TextureCube environment_cubemap : register(t0);
SamplerState s0 : register(s0);

PS_OUTPUT main_ps(VS_OUTPUT input) : SV_TARGET
{
	PS_OUTPUT output;

	const float PI = 3.14159265359f;

	float3 normal = normalize(input.local_pos);
	float3 irradiance = float3(0.0f, 0.0f, 0.0f);

	float3 up = float3(0.0f, 1.0f, 0.0f);
	float3 right = cross(up, normal);
	up = cross(normal, right);

	float sample_delta = 0.025f;
	float nr_samples = 0.0f;

	for (float phi = 0.0f; phi < 2.0f * PI; phi += sample_delta)
	{
		for (float theta = 0.0f; theta < 0.5f * PI; theta += sample_delta)
		{
			float cos_theta = cos(theta);
			float sin_theta = sin(theta);

			float3 tangent_sample = float3(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);
			float3 sample_vec = tangent_sample.x * right + tangent_sample.y * up + tangent_sample.z * normal;

			irradiance += environment_cubemap.Sample(s0, sample_vec).rgb * cos_theta * sin_theta;

			nr_samples++;
		}
	}

	irradiance = PI * irradiance * (1.0f / float(nr_samples));
	
	output.color = float4(irradiance.rgb, 1.0f);

	return output;
}

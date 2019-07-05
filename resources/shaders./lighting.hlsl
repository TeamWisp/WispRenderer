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
#ifndef __LIGHTING_HLSL__
#define __LIGHTING_HLSL__

#define CALLINGPASS_SHADOWS 0
#define CALLINGPASS_REFLECTIONS 1
#define CALLINGPASS_PATHTRACING 2
#define CALLINGPASS_FULLRAYTRACING 3

#include "dxr_shadow_functions.hlsl"

struct Light
{
	float3 pos;			//Position in world space for spot & point
	float rad;			//Radius for point, height for spot

	float3 col;			//Color
	uint tid;			//Type id; light_type_x

	float3 dir;			//Direction for spot & directional
	float ang;			//Angle for spot; in radians

	float3 padding;
	float light_size;
};

StructuredBuffer<Light> lights : LIGHTS_REGISTER;

static uint light_type_point = 0;
static uint light_type_directional = 1;
static uint light_type_spot = 2;

float calc_attenuation(Light light, float3 L, float light_dist)
{
	uint tid = light.tid & 3;

	float min_cos = cos(light.ang);
	float max_cos = lerp(min_cos, 1, 0.5f);
	float cos_angle = dot(light.dir, L);
	return lerp(smoothstep(min_cos, max_cos, cos_angle), 1.0f - smoothstep(0, light.rad, light_dist), tid != light_type_spot);
}

//Copied version for testing stuff
float3 shade_light(float3 pos, float3 V, float3 albedo, float3 normal, float metallic, float roughness, Light light)
{
	uint tid = light.tid & 3;

	//Light direction (constant with directional, position dependent with other)
	float3 L = (lerp(light.pos - pos, light.dir, tid == light_type_directional));
	float light_dist = length(L);
	L /= light_dist;

	float attenuation = calc_attenuation(light, L, light_dist);

	float3 radiance = light.col * attenuation;

	float3 lighting = BRDF(L, V, normal, metallic, roughness, albedo, radiance);

	return lighting;
}

float3 shade_pixel(float3 pos, float3 V, float3 albedo, float metallic, float roughness, float3 emissive, float3 normal, float3 irradiance, float ao, float3 reflection, float2 brdf, float3 shadow_factor, bool uses_luminance)
{
	float3 res = float3(0.0f, 0.0f, 0.0f);

	uint light_count = lights[0].tid >> 2;	//Light count is stored in 30 upper-bits of first light

	if(!uses_luminance)
	{
		for (uint i = 0; i < light_count; i++)
		{
			res += shade_light(pos, V, albedo, normal, metallic, roughness, lights[i]);
		}
	}
	else
	{
		res = albedo * shadow_factor;
	}

	// Ambient Lighting using Irradiance for Diffuse
	float3 kS = F_SchlickRoughness(max(dot(normal, V), 0.0f), metallic, albedo, roughness);
	float3 kD = 1.0f - kS;
	kD *= 1.0f - metallic;

	float3 diffuse = irradiance * albedo;

	// Image-Based Lighting using Prefiltered Environment Map and BRDF LUT for Specular
	float3 prefiltered_color = reflection;
	float2 sampled_brdf = brdf;
	
	float3 specular = prefiltered_color * (kS * sampled_brdf.x + sampled_brdf.y);
	//float3 specular = reflection * kS;
	
	float3 ambient = (kD * diffuse + specular) * ao;

	return ambient + res + emissive;
}

float3 shade_light(float3 pos, float3 V, float3 albedo, float3 normal, float metallic, float roughness, Light light, inout uint rand_seed, uint shadow_sample_count, uint depth, uint calling_pass)
{
	uint tid = light.tid & 3;

	//Light direction (constant with directional, position dependent with other)
	float3 L = (lerp(light.pos - pos, light.dir, tid == light_type_directional));
	float light_dist = length(L);
	L /= light_dist;

	float attenuation = calc_attenuation(light, L, light_dist);

	// Maybe change hard-coded 100000 to be dynamic according to the scene size?
	float t_max = lerp(light_dist, 100000, tid == light_type_directional);

	float3 radiance = light.col * attenuation;

	float3 lighting = BRDF(L, V, normal, metallic, roughness, albedo, radiance);

	float3 wpos = pos + (normal * EPSILON);
	
	float shadow_factor = GetShadowFactor(wpos, L, light.light_size, t_max, shadow_sample_count, depth, calling_pass, rand_seed);

	lighting *= shadow_factor;

	return lighting;
}

float3 shade_pixel(float3 pos, float3 V, float3 albedo, float metallic, float roughness, float3 emissive, float3 normal, inout uint rand_seed, uint shadow_sample_count, uint depth, uint calling_pass)
{
	uint light_count = lights[0].tid >> 2;	//Light count is stored in 30 upper-bits of first light

	float3 res = float3(0, 0, 0);

	[unroll]
	for (uint i = 0; i < light_count; i++)
	{
		res += shade_light(pos, V, albedo, normal, metallic, roughness, lights[i], rand_seed, shadow_sample_count, depth, calling_pass);
	}

	return res + emissive;
}

float4 DoShadowAllLights(float3 wpos, float3 V, float3 normal, float metallic, float roughness, float3 albedo, uint shadow_sample_count, uint depth, uint calling_pass, inout float rand_seed)
{
	uint light_count = lights[0].tid >> 2;	//Light count is stored in 30 upper-bits of first light

	float4 res = float4(0.0, 0.0, 0.0, 0.0);
	uint sampled_lights = 0;

	for (uint i = 0; i < light_count; i++)
	{
		// Get light and light type
		Light light = lights[i];
		uint tid = light.tid & 3;

		//Light direction (constant with directional, position dependent with other)
		float3 L = (lerp(light.pos - wpos, light.dir, tid == light_type_directional));
		float light_dist = length(L);
		L /= light_dist;

		float dir_dot = dot(L, normal);

		if (dir_dot < 0.0f)
		{
			continue;
		}

		float attenuation = calc_attenuation(light, L, light_dist);

		if (light_dist > light.rad && tid != light_type_directional)
		{
			continue;
		}

		float3 radiance = attenuation * light.col;

		float3 lighting = BRDF(L, V, normal, metallic, roughness, float3(1.0, 1.0, 1.0), radiance) /* / max(albedo, float3(0.001, 0.001, 0.001)) */;

		// Get maxium ray length (depending on type)
		float t_max = lerp(light_dist, 100000, tid == light_type_directional);

		// Add shadow factor to final result
		float shadow = GetShadowFactor(wpos, L, light.light_size, t_max, shadow_sample_count, depth, calling_pass, rand_seed);

		res.w += shadow;

		res.rgb += lighting * shadow;

		sampled_lights++;
	}

	// return final res
	res.w = res.w / float(sampled_lights);

	return res;
}

#endif //__LIGHTING_HLSL__

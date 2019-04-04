#include "shadow_ray.hlsl"

struct Light
{
	float3 pos;			//Position in world space for spot & point
	float rad;			//Radius for point, height for spot

	float3 col;			//Color
	uint tid;			//Type id; light_type_x

	float3 dir;			//Direction for spot & directional
	float ang;			//Angle for spot; in radians
};

StructuredBuffer<Light> lights : LIGHTS_REGISTER;

static uint light_type_point = 0;
static uint light_type_directional = 1;
static uint light_type_spot = 2;

//Copied version for testing stuff
float3 shade_light(float3 pos, float3 V, float3 albedo, float3 normal, float metallic, float roughness, Light light)
{
	uint tid = light.tid & 3;

	//Light direction (constant with directional, position dependent with other)
	float3 L = (lerp(light.pos - pos, light.dir, tid == light_type_directional));
	float light_dist = length(L);
	L /= light_dist;

	//Spot intensity (only used with spot; but always calculated)
	float min_cos = cos(light.ang);
	float max_cos = lerp(min_cos, 1, 0.5f);
	float cos_angle = dot(light.dir, L);
	float spot_intensity = lerp(smoothstep(min_cos, max_cos, cos_angle), 1, tid != light_type_spot);

	//Attenuation & spot intensity (only used with point or spot)
	float attenuation = lerp(1.0f - smoothstep(0, light.rad, light_dist), 1, tid == light_type_directional);

	float3 radiance = (light.col * spot_intensity) * attenuation;

	float3 lighting = BRDF(L, V, normal, metallic, roughness, albedo, radiance);

	return lighting;
}

float3 shade_pixel(float3 pos, float3 V, float3 albedo, float metallic, float roughness, float3 normal, float3 irradiance, float3 reflection, float2 brdf, float shadow_factor)
{
	float3 res = float3(0.0f, 0.0f, 0.0f);

	uint light_count = lights[0].tid >> 2;	//Light count is stored in 30 upper-bits of first light

	res = float3(0.0f, 0.0f, 0.0f);

	for (uint i = 0; i < light_count; i++)
	{
		res += shade_light(pos, V, albedo, normal, metallic, roughness, lights[i]) * shadow_factor;
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
	
	float3 ambient = (kD * diffuse + specular) * 1.0f; //Replace 1.0f with AO, when we have it.

	return ambient + (res * shadow_factor);
}

float3 shade_light(float3 pos, float3 V, float3 albedo, float3 normal, float metallic, float roughness, Light light, inout uint rand_seed, uint depth)
{
	uint tid = light.tid & 3;

	//Light direction (constant with directional, position dependent with other)
	float3 L = (lerp(light.pos - pos, light.dir, tid == light_type_directional));
	float light_dist = length(L);
	L /= light_dist;

	//Spot intensity (only used with spot; but always calculated)
	float min_cos = cos(light.ang);
	float max_cos = lerp(min_cos, 1, 0.5f);
	float cos_angle = dot(light.dir, L);
	float spot_intensity = lerp(smoothstep(min_cos, max_cos, cos_angle), 1, tid != light_type_spot);

	//Attenuation & spot intensity (only used with point or spot)
	float attenuation = lerp(1.0f - smoothstep(0, light.rad, light_dist), 1, tid == light_type_directional);

	// Maybe change hard-coded 100000 to be dynamic according to the scene size?
	float t_max = lerp(light_dist, 100000, tid == light_type_directional);

	float3 radiance = (light.col * spot_intensity) * attenuation;

	float3 lighting = BRDF(L, V, normal, metallic, roughness, albedo, radiance);

	float3 wpos = pos + (normal * EPSILON);

	// Offset shadow ray direction to get soft-shadows
	float shadow_factor = GetShadowFactor(wpos, L, t_max, depth, rand_seed);

	lighting *= shadow_factor;

	return lighting;
}

float3 shade_pixel(float3 pos, float3 V, float3 albedo, float metallic, float roughness, float3 normal, inout uint rand_seed, uint depth)
{
	uint light_count = lights[0].tid >> 2;	//Light count is stored in 30 upper-bits of first light

	float3 res = float3(0, 0, 0);

	[unroll]
	for (uint i = 0; i < light_count; i++)
	{
		res += shade_light(pos, V, albedo, normal, metallic, roughness, lights[i], rand_seed, depth);
	}

	return res;
}

float DoShadowAllLights(float3 wpos, uint depth, inout float rand_seed)
{
	uint light_count = lights[0].tid >> 2;	//Light count is stored in 30 upper-bits of first light

	float res = 0;

	for (uint i = 0; i < light_count; i++)
	{
		// Get light and light type
		Light light = lights[i];
		uint tid = light.tid & 3;

		//Light direction (constant with directional, position dependent with other)
		float3 L = (lerp(light.pos - wpos, light.dir, tid == light_type_directional));
		float light_dist = length(L);
		L /= light_dist;

		// Get maxium ray length (depending on type)
		float t_max = lerp(light_dist, 100000, tid == light_type_directional);

		// Add shadow factor to final result
		res += GetShadowFactor(wpos, L, t_max, depth + 1, rand_seed);
	}

	// return final res
	return res / float(light_count);
}

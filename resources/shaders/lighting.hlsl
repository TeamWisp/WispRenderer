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
	float cos_angle = dot(light.dir, -L);
	float spot_intensity = lerp(smoothstep(min_cos, max_cos, cos_angle), 1, tid != light_type_spot);

	//Attenuation & spot intensity (only used with point or spot)
	float attenuation = lerp(1.0f - smoothstep(0, light.rad, light_dist), 1, tid == light_type_directional);
	float3 radiance = (light.col * spot_intensity) * attenuation;

	float3 lighting = BRDF(L, V, normal, metallic, roughness, albedo, radiance, light.col);

	return lighting;

}

float3 shade_pixel(float3 pos, float3 V, float3 albedo, float metallic, float roughness, float3 normal)
{
	uint light_count = lights[0].tid >> 2;	//Light count is stored in 30 upper-bits of first light

	float ambient = 0.1f;
	float3 res = float3(ambient, ambient, ambient);

	Light hardcoded_light;
	hardcoded_light.pos = float3(0.0f, 0.0f, 0.0f);
	hardcoded_light.rad = 0.0f;
	hardcoded_light.col = float3(10.0f, 10.0f, 10.0f);
	hardcoded_light.tid = 1;
	hardcoded_light.dir = float3(0.0f, -1.0f, 0.0f);
	hardcoded_light.ang = 0.0f;

	res += shade_light(pos, V, albedo, normal, metallic, roughness, hardcoded_light);

	//for (uint i = 0; i < light_count; i++)
	//{
	//	res += shade_light(pos, V, albedo, normal, metallic, roughness, lights[i]);
	//}

	return res * albedo;
}
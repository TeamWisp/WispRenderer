#define M_PI 3.14159265358979

struct HitInfo
{
	float3 color;
	unsigned int seed;
	float3 origin;
	unsigned int depth;
};

float4 TraceColorRay(float3 origin, float3 direction, unsigned int depth, unsigned int seed)
{
	if (depth >= MAX_RECURSION)
	{
		//return skybox.SampleLevel(s0, SampleSphericalMap(direction), 0);
		return float4(0, 0, 0, 0);
	}

	// Define a ray, consisting of origin, direction, and the min-max distance values
	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = direction;
	ray.TMin = 0;
	ray.TMax = 1.0e38f;

	HitInfo payload = { float3(1, 1, 1), seed, origin, depth };

	// Trace the ray
	TraceRay(
		Scene,
		RAY_FLAG_NONE,
		~0, // InstanceInclusionMask
		0, // RayContributionToHitGroupIndex
		0, // MultiplierForGeometryContributionToHitGroupIndex
		0, // miss shader index
		ray,
		payload);

	return float4(payload.color, 1);
}

// NVIDIA's luminance function
inline float luminance(float3 rgb)
{
    return dot(rgb, float3(0.2126f, 0.7152f, 0.0722f));
}

// NVIDIA's probability function
float probabilityToSampleDiffuse(float3 difColor, float3 specColor)
{
	float lumDiffuse = max(0.01f, luminance(difColor.rgb));
	float lumSpecular = max(0.01f, luminance(specColor.rgb));
	return lumDiffuse / (lumDiffuse + lumSpecular);
}

float3 ggxDirect(float3 hit_pos, float3 fN, float3 N, float3 V, float3 albedo, float metal, float roughness, unsigned int seed, unsigned int depth)
{
	// #################### GGX #####################
	uint light_count = lights[0].tid >> 2;
	if (light_count < 1) return 0;

	int light_to_sample = min(int(nextRand(seed) * light_count), light_count - 1);
	Light light = lights[light_to_sample];

	float3 L = 0;
	float max_light_dist = 0;
	float3 light_intensity = 0;
	{
		// Calculate light properties
		uint tid = light.tid & 3;

		//Light direction (constant with directional, position dependent with other)
		L = (lerp(light.pos - hit_pos, light.dir, tid == light_type_directional));
		float light_dist = length(L);
		L /= light_dist;

		//Spot intensity (only used with spot; but always calculated)
		float min_cos = cos(light.ang);
		float max_cos = lerp(min_cos, 1, 0.5f);
		float cos_angle = dot(light.dir, L);
		float spot_intensity = lerp(smoothstep(min_cos, max_cos, cos_angle), 1, tid != light_type_spot);

		//Attenuation & spot intensity (only used with point or spot)
		float attenuation = lerp(1.0f - smoothstep(0, light.rad, light_dist), 1, tid == light_type_directional);

		light_intensity = (light.col * spot_intensity) * attenuation;
		max_light_dist = lerp(light_dist, 100000, tid == light_type_directional);
	}

	float3 H = normalize(V + L);

	// Shadow
	float shadow_mult = float(light_count) * GetShadowFactor(hit_pos + (L * EPSILON), L, max_light_dist, depth, seed);

	// Compute some dot products needed for shading
	float NdotV = saturate(dot(fN, V));
	float NdotL = saturate(dot(fN, L));
	float NdotH = saturate(dot(fN, H));
	float LdotH = saturate(dot(L, H));

	// D = Normal distribution (Distribution of the microfacets)
	float D = D_GGX(NdotH, max(0.05, roughness)); 
	// G = Geometric shadowing term (Microfacets shadowing)
	float G = G_SchlicksmithGGX(NdotL, NdotV, max(0.05, roughness));
	// F = Fresnel factor (Reflectance depending on angle of incidence)
	float3 F = F_Schlick(LdotH, metal, albedo);
	//float3 F = F_ShlickSimple(metal, LdotH);

	float3 kS = F_SchlickRoughness(NdotV, metal, albedo, roughness);
	float3 kD = (1.f - kS) * (1.0 - metal);
	float3 spec = (D * F * G) / (4.0 * NdotV * NdotL + 0.001f);

	float3 lighting = (light_intensity * (NdotL * spec + NdotL * albedo / M_PI));

	return (shadow_mult * lighting);
}

float3 ggxIndirect(float3 hit_pos, float3 fN, float3 N, float3 V, float3 albedo, float metal, float roughness, float ao, unsigned int seed, unsigned int depth)
{
	// #################### GGX #####################
	float diffuse_probability = probabilityToSampleDiffuse(albedo, metal);
	float choose_diffuse = (nextRand(seed) < diffuse_probability);

	// Diffuse lobe
	if (choose_diffuse)
	{
		float3 color = (albedo / diffuse_probability) * ao;

#ifdef RUSSIAN_ROULETTE
		// ############## RUSSIAN ROULETTE ###############
		// Russian Roulette
		// Randomly terminate a path with a probability inversely equal to the throughput
		float p = max(color.x, max(color.y, color.z));
		// Add the energy we 'lose' by randomly terminating paths
		color *= 1 / p;
		if (nextRand(seed) > p) {
			return 0;
		}
#endif

		// ##### BOUNCE #####
		nextRand(seed);
		const float3 rand_dir = getCosHemisphereSample(seed, N);
		float3 irradiance = TraceColorRay(hit_pos + (EPSILON * N), rand_dir, depth, seed);
		if (dot(N, rand_dir) <= 0.0f) irradiance = float3(0, 0, 0);


		return irradiance * color;
	}
	else
	{
		nextRand(seed);
		float3 H = getGGXMicrofacet(seed, roughness, N);

		// ### BRDF ###
		float3 L = normalize(2.f * dot(V, H) * H - V);

		// Compute some dot products needed for shading
		float NdotV = saturate(dot(N, V));
		float NdotL = saturate(dot(N, L));
		float NdotH = saturate(dot(N, H));
		float LdotH = saturate(dot(L, H));

		// D = Normal distribution (Distribution of the microfacets)
		float D = D_GGX(NdotH, max(0.05, roughness)); 
		// G = Geometric shadowing term (Microfacets shadowing)
		float G = G_SchlicksmithGGX(NdotL, NdotV, max(0.05, roughness));
		// F = Fresnel factor (Reflectance depending on angle of incidence)
		float3 F = F_Schlick(NdotH, metal, albedo);
 
		float3 spec = (D * F * G) / ((4.0 * NdotL * NdotV + 0.001f));
		float  ggx_probability = D * NdotH / (4 * LdotH);

		float3 color = (NdotL * spec / (ggx_probability * (1.0f - diffuse_probability)));

#ifdef RUSSIAN_ROULETTE
		// ############## RUSSIAN ROULETTE ###############
		// Russian Roulette
		// Randomly terminate a path with a probability inversely equal to the throughput
		float p = max(color.x, max(color.y, color.z));
		// Add the energy we 'lose' by randomly terminating paths
		color *= 1 / p;
		if (nextRand(seed) > p) {
			return 0;
		}
#endif

		// #### BOUNCE ####
		float3 irradiance = TraceColorRay(hit_pos + (EPSILON * N), L, depth, seed);
		if (dot(N, L) <= 0.0f) irradiance = float3(0, 0, 0);

		return color * irradiance;
	}
}

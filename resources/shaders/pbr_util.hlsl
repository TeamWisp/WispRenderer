static const float PI = 3.14159265359;
 
// Based omn http://byteblacksmith.com/improvements-to-the-canonical-one-liner-glsl-rand-for-opengl-es-2-0/
float random(float2 co)
{
	float a = 12.9898;
	float b = 78.233;
	float c = 43758.5453;
	float dt = dot(co.xy, float2(a, b));
	float sn = fmod(dt, 3.14);
	return frac(sin(sn) * c);
}
 
// Radical inverse based on http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float2 hammersley2d(uint i, uint N)
{
	uint bits = (i << 16u) | (i >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	float rdi = float(bits) * 2.3283064365386963e-10;
	return float2(float(i) / float(N), rdi);
}
 
// Based on http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_slides.pdf
float3 importanceSample_GGX(float2 Xi, float roughness, float3 normal)
{
	// Maps a 2D point to a hemisphere with spread based on roughness
	float alpha = roughness * roughness;
	float phi = 2.f * PI * Xi.x + random(normal.xz) * 0.1;
	float cosTheta = sqrt((1.f - Xi.y) / (1.f + (alpha*alpha - 1.f) * Xi.y));
	float sinTheta = sqrt(1.f - cosTheta * cosTheta);
	
	float3 H;
	H.x = sinTheta * cos(phi);
	H.y = sinTheta * sin(phi);
	H.z = cosTheta;
 
	// Tangent space
	float3 up = abs(normal.z) < 0.999 ? float3(0.f, 0.f, 1.f) : float3(1.f, 0.f, 0.f);
	float3 tangentX = normalize(cross(up, normal));
	float3 tangentY = cross(normal, tangentX);
 
	// Convert to world Space
	return tangentX * H.x + tangentY * H.y + normal * H.z;
}
 
// Normal distribution
float D_GGX(float dotNH, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2)/(PI * denom * denom); 
}
 
// Geometric Shadowing function
float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;
	float GL = dotNL / (dotNL * (1.0 - k) + k);
	float GV = dotNV / (dotNV * (1.0 - k) + k);
	return GL * GV;
}
 
 // Fresnel function
float3 F_Schlick(float cos_theta, float metallic, float3 material_color)
{
	float3 F0 = lerp(float3(0.04, 0.04, 0.04), material_color, metallic); // * material.specular
	float3 F = F0 + (1.0 - F0) * pow(1.0 - cos_theta, 5.0); 
	return F;
}
 
float3 F_SchlickRoughness(float cos_theta, float metallic, float3 material_color, float roughness)
{
	float3 F0 = lerp(float3(0.04, 0.04, 0.04), material_color, metallic); // * material.specular
	float3 F = F0 + (max((1.0 - roughness), F0) - F0) * pow(1.0 - cos_theta, 5.0);
	return F;
}
 
float3 BRDF(float3 L, float3 V, float3 N, float metallic, float roughness, float3 albedo, float3 radiance, float3 light_color)
{
	// Precalculate vectors and dot products	
	float3 H = normalize(V + L);
	float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);
	float dotLH = clamp(dot(L, H), 0.0, 1.0);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);
 
	// Light color fixed
	float3 color = float3(0.0, 0.0, 0.0);
 
	if (dotNL > 0.0)
	{
		float rroughness = max(0.05, roughness);
		// D = Normal distribution (Distribution of the microfacets)
		float D = D_GGX(dotNH, roughness); 
		// G = Geometric shadowing term (Microfacets shadowing)
		float G = G_SchlicksmithGGX(dotNL, dotNV, roughness);
		// F = Fresnel factor (Reflectance depending on angle of incidence)
		float3 F = F_Schlick(dotNV, metallic, albedo);
 
		float3 spec = D * F * G / (4.0 * dotNL * dotNV);
 
		float3 kS = F;
		float3 kD = float3(1, 1, 1) - kS;
		kD *= 1.0 - metallic;
 
		float3 nominator = D * G * F;
		float denominator = 4.0 * dotNV * dotNL;
		float3 specular = nominator / max(denominator, 0.001);
 
		float NdotL = max(dot(N, L), 0.0);
		color += (kD * albedo / PI + specular) * radiance * dotNL;
 
		//color += spec * dotNL * lightColor;
	}
 
	return color;
}
 
static const float2 inv_atan = float2(0.1591f, 0.3183f);
float2 SampleSphericalMap(float3 v)
{
	float2 uv = float2(atan2(v.z, v.x), asin(v.y * -1.f));
	uv *= inv_atan;
	uv += 0.5f;
	return uv;
}

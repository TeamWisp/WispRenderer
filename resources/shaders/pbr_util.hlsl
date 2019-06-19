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
#ifndef __PBR_UTILS_HLSL__
#define __PBR_UTILS_HLSL__

#include "math.hlsl"

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
float2 hammersley2d(uint i, uint num)
{
	uint bits = (i << 16u) | (i >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	float rdi = float(bits) * 2.3283064365386963e-10;
	return float2(float(i) / float(num), rdi);
}
 
// Based on http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_slides.pdf
float3 importanceSample_GGX(float2 Xi, float roughness, float3 normal)
{
	// Maps a 2D point to a hemisphere with spread based on roughness
	float alpha = roughness * roughness;
	//float phi = 2.f * M_PI * Xi.x + random(normal.xz) * 0.1;
	float phi = 2.f * M_PI * Xi.x;
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
	return normalize(tangentX * H.x + tangentY * H.y + normal * H.z);
}
 
// Normal distribution
float D_GGX(float dotNH, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2)/(M_PI * denom * denom);
}

// Get a GGX half vector / microfacet normal, sampled according to the distribution computed by
//     the function ggxNormalDistribution() above.  
//
// When using this function to sample, the probability density is pdf = D * NdotH / (4 * HdotV)
float3 getGGXMicrofacet(inout uint randSeed, float roughness, float3 hitNorm)
{
	// Get our uniform random numbers
	float2 randVal = float2(nextRand(randSeed), nextRand(randSeed));

	// Get an orthonormal basis from the normal
	float3 B = getPerpendicularVector(hitNorm);
	float3 T = cross(B, hitNorm);

	// GGX NDF sampling
	float a2 = roughness * roughness;
	float cosThetaH = sqrt(max(0.0f, (1.0 - randVal.x) / ((a2 - 1.0) * randVal.x + 1)));
	float sinThetaH = sqrt(max(0.0f, 1.0f - cosThetaH * cosThetaH));
	float phiH = randVal.y * M_PI * 2.0f;

	// Get our GGX NDF sample (i.e., the half vector)
	return T * (sinThetaH * cos(phiH)) + B * (sinThetaH * sin(phiH)) + hitNorm * cosThetaH;
}
 
// Geometric Shadowing function
float G_SchlicksmithGGX(float NdotL, float NdotV, float roughness)
{
	float r = (roughness + 1.0f);
	float k = (r*r) / 8.0f;
	float GL = NdotL / (NdotL * (1.0f - k) + k);
	float GV = NdotV / (NdotV * (1.0f - k) + k);
	return GL * GV;
}

float GeometrySchlickGGX_IBL(float NdotV, float roughness_squared)
{
	// Different k for IBL
	float k = roughness_squared / 2.0;

	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith_IBL(float NdotV, float NdotL, float roughness)
{
	float roughness_squared = roughness * roughness;

	float ggx2 = GeometrySchlickGGX_IBL(NdotV, roughness_squared);
	float ggx1 = GeometrySchlickGGX_IBL(NdotL, roughness_squared);

	return ggx1 * ggx2;
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
	float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), material_color, metallic); // * material.specular
	float3 F = F0 + (max(float3(1.0f - roughness, 1.0f - roughness, 1.0f - roughness), F0) - F0) * pow(1.0f - cos_theta, 5.0f);
	return F;
}
 
float3 BRDF(float3 L, float3 V, float3 N, float metallic, float roughness, float3 albedo, float3 radiance)
{
	// Precalculate vectors and dot products	
	float3 H = normalize(V + L);
	float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);
 
	// Light color fixed
	float3 color = float3(0.0, 0.0, 0.0);
 
	if (dotNL > 0.0)
	{
		float rroughness = max(0.05f, roughness);
		// D = Normal distribution (Distribution of the microfacets)
		float D = D_GGX(dotNH, roughness); 
		// G = Geometric shadowing term (Microfacets shadowing)
		float G = G_SchlicksmithGGX(dotNL, dotNV, roughness);
		// F = Fresnel factor (Reflectance depending on angle of incidence)
		float3 F = F_Schlick(dotNH, metallic, albedo);
 
		float3 spec = (D * F * G) / ((4.0 * dotNL * dotNV + 0.001f));
 
		float3 kD = (float3(1, 1, 1) - F) * (1.0 - metallic);
 
		color += (kD * albedo / M_PI + spec) * radiance * dotNL;
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

#endif //__PBR_UTILS_HLSL__
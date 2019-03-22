#ifndef __UTILS_HLSL__
#define __UTILS_HLSL__

// Initialize random seed
uint initRand(uint val0, uint val1, uint backoff = 16)
{
	uint v0 = val0, v1 = val1, s0 = 0;

	[unroll]
	for (uint n = 0; n < backoff; n++)
	{
		s0 += 0x9e3779b9;
		v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
		v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
	}
	return v0;
}

// Get 'random' value [0, 1]
float nextRand(inout uint s)
{
	s = (1664525u * s + 1013904223u);
	return float(s & 0x00FFFFFF) / float(0x01000000);
}

float3 getPerpendicularVector(float3 u)
{
	float3 a = abs(u);
	uint xm = ((a.x - a.y)<0 && (a.x - a.z)<0) ? 1 : 0;
	uint ym = (a.y - a.z)<0 ? (1 ^ xm) : 0;
	uint zm = 1 ^ (xm | ym);
	return cross(u, float3(xm, ym, zm));
}
// Get a cosine-weighted random vector centered around a specified normal direction.
float3 getCosHemisphereSample(inout uint randSeed, float3 hitNorm)
{
	// Get 2 random numbers to select our sample with
	float2 randVal = float2(nextRand(randSeed), nextRand(randSeed));

	// Cosine weighted hemisphere sample from RNG
	float3 bitangent = getPerpendicularVector(hitNorm);
	float3 tangent = cross(bitangent, hitNorm);
	float r = sqrt(randVal.x);
	float phi = 2.0f * 3.14159265f * randVal.y;

	// Get our cosine-weighted hemisphere lobe sample direction
	return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNorm.xyz * sqrt(max(0.0, 1.0f - randVal.x));
}

float origin() { return 1.0f / 32.0f; }
float float_scale() { return 1.0f / 65536.0f; }
float int_scale() { return 256.0f; }
float magic(float x, float nx) {return asfloat(asuint(x) + int(nx * 256) * (int(x >= 0) * 2 - 1));}

 #define NVIDIA
 // Normal points outward for rays exiting the surface, else is flipped.
 float3 offset_ray(const float3 p, const float3 n)
 {
	 #ifdef NVIDIA
 	int3 of_i = {int_scale() * n.x, int_scale() * n.y, int_scale() * n.z};
 
	float3 p_i = {
	asfloat(asint(p.x)+((p.x < 0) ? -of_i.x : of_i.x)),
	asfloat(asint(p.y)+((p.y < 0) ? -of_i.y : of_i.y)),
	asfloat(asint(p.z)+((p.z < 0) ? -of_i.z : of_i.z))};
	
	float3 output = float3(
	(abs(p.x) < origin() ? p.x+ float_scale()*n.x : p_i.x) * 1.0f,
	(abs(p.y) < origin() ? p.y+ float_scale()*n.y : p_i.y) * 1.0f,
	(abs(p.z) < origin() ? p.z+ float_scale()*n.z : p_i.z) * 1.0f);
	float3 difference = output - p;
	// return float3(
	// (abs(p.x) < origin() ? p.x+ float_scale()*n.x : p_i.x) * 1.0f,
	// (abs(p.y) < origin() ? p.y+ float_scale()*n.y : p_i.y) * 1.0f,
	// (abs(p.z) < origin() ? p.z+ float_scale()*n.z : p_i.z) * 1.0f);
	return p + difference *10000.0f;
	#endif 
	#ifdef NELIS

	 return float3(	magic(p.x, n.x), magic(p.y, n.y), magic(p.z, n.z));
	
	#endif
	return float3(p + n * 0.100750000f);
}

#endif // __UTILS_HLSL__

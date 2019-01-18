float3 Vignette(float3 input, float2 pos, float radius, float softness, float strength)
{
	float len = length(pos * 2 - 1);
	float vignette = smoothstep(radius, radius -  softness, len);
	return lerp(input, input * vignette, strength);
}

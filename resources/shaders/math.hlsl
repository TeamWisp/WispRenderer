#ifndef __MATH_HLSL__
#define __MATH_HLSL__

#define M_PI 3.14159265358979

float linterp(float t) {
	return saturate(1.0 - abs(2.0 * t - 1.0));
}

float remap(float t, float a, float b) 
{
	return saturate((t - a) / (b - a));
}

float4 spectrum_offset(float t) 
{
	float4 ret;
	float lo = step(t, 0.5);
	float hi = 1.0 - lo;
	float w = linterp(remap(t, 1.0 / 6.0, 5.0 / 6.0));
	ret = float4(lo, 1.0, hi, 1.) * float4(1.0 - w, w, 1.0 - w, 1.);

	return pow(ret, float4(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));
}

#endif //__MATH_HLSL__
#include "fxaa.hlsl"

float2 barrelDistortion(float2 coord, float amt) {
	float2 cc = coord - 0.5;
	float dist = dot(cc, cc);
	return coord + cc * dist * amt;
}

float sat( float t )
{
	return clamp( t, 0.0, 1.0 );
}

float linterp( float t ) {
	return sat( 1.0 - abs( 2.0*t - 1.0 ) );
}

float remap( float t, float a, float b ) {
	return sat( (t - a) / (b - a) );
}

float4 spectrum_offset( float t ) {
	float4 ret;
	float lo = step(t,0.5);
	float hi = 1.0-lo;
	float w = linterp( remap( t, 1.0/6.0, 5.0/6.0 ) );
	ret = float4(lo,1.0,hi, 1.) * float4(1.0-w, w, 1.0-w, 1.);

	return pow( ret, float4(1.0/2.2, 1.0/2.2, 1.0/2.2, 1.0/2.2) );
}

static const int num_iter = 11;
static const float reci_num_iter_f = 1.0 / float(num_iter);

// Old naive chromatic aberration.
float4 ChromaticAberration(Texture2D tex, SamplerState s, float2 uv, float strength) {
	float2 r_offset = float2(strength, 0);
	float2 g_offset = float2(-strength, 0);
	float2 b_offset = float2(0, 0);
 
	float4 r = float4(1, 1, 1, 1);
	r.x = tex.SampleLevel(s, uv + r_offset, 0).x;
	r.y = tex.SampleLevel(s, uv + g_offset, 0).y;
	r.z = tex.SampleLevel(s, uv + b_offset, 0).z;
	r.a = tex.SampleLevel(s, uv, 0).a;
 
	return r;
}

// Zoom into a image.
float2 ZoomUV(float2 uv, float zoom)
{
	return (uv * zoom) + ((1 - (zoom)) / 2);
}

float4 ChromaticAberrationV2(Texture2D tex, SamplerState s, float2 uv, float strength, float zoom) {
	uv = ZoomUV(uv, zoom);
	float4 sumcol = 0.0;
	float4 sumw = 0.0;	

	float2 resolution;
	tex.GetDimensions(resolution.x, resolution.y);

	for ( int i=0; i<num_iter;++i )
	{
		float t = float(i) * reci_num_iter_f;
		float4 w = spectrum_offset( t );
		sumw += w;
		//sumcol += w * tex.SampleLevel(s, barrelDistortion(uv, 0.6 * strength*t ), 0);
		sumcol += w * SampleFXAA(tex, s, barrelDistortion(uv, 0.6 * strength*t) * resolution, resolution);
	}
 
	return sumcol / sumw;
}

float2 BarrelDistortUV(float2 uv, float kcube)
{
	float k = -0.15;
   
	float r2 = (uv.x-0.5) * (uv.x-0.5) + (uv.y-0.5) * (uv.y-0.5);       
	float f = 0;

	//only compute the cubic distortion if necessary
	if( kcube == 0.0)
	{
			f = 1 + r2 * k;
	}
	else
	{
			f = 1 + r2 * (k + kcube * sqrt(r2));
	};
   
	// get the right pixel for the current position
	float x = f * (uv.x - 0.5) + 0.5;
	float y = f * (uv.y - 0.5) + 0.5;

	return float2(x, y);
}

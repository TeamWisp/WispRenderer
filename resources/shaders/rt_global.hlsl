#ifndef __RT_GLOBAL_HLSL__
#define __RT_GLOBAL_HLSL__

#define MAX_RECURSION 3
//#define FOUR_X_A
//#define PATH_TRACING
//#define DEPTH_OF_FIELD
#define EPSILON 0.1
#define MAX_SHADOW_SAMPLES 4
#define RUSSIAN_ROULETTE

#ifdef FALLBACK
	#undef MAX_RECURSION
	#define MAX_RECURSION 1

	#define NO_SHADOWS
#endif

RaytracingAccelerationStructure Scene : register(t0, space0);

#endif //__RT_GLOBAL_HLSL__
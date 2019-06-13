#ifndef __DXR_GLOBAL_HLSL__
#define __DXR_GLOBAL_HLSL__

#define MAX_RECURSION 2
//#define FOUR_X_A
//#define PATH_TRACING
//#define DEPTH_OF_FIELD
#define EPSILON 2.1
#define SOFT_SHADOWS
#define MAX_SHADOW_SAMPLES 1

//#define PERFECT_MIRROR_REFLECTIONS
#define GROUND_TRUTH_REFLECTIONS
#define MAX_GT_REFLECTION_SAMPLES 4
//#define DISABLE_SPATIAL_RECONSTRUCTION

#define RUSSIAN_ROULETTE

#ifdef FALLBACK
	#undef MAX_RECURSION
	#define MAX_RECURSION 1

	
	//#define NO_SHADOWS
#endif

RaytracingAccelerationStructure Scene : register(t0, space0);

#endif //__DXR_GLOBAL_HLSL__
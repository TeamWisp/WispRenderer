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
#define MAX_GT_REFLECTION_SAMPLES 16
//#define DISABLE_SPATIAL_RECONSTRUCTION

#define RUSSIAN_ROULETTE
#define NO_PATH_TRACED_NORMALS
#define CALC_BITANGENT // calculate bitangent in the shader instead of using the bitangent uploaded

#ifdef FALLBACK
	#undef MAX_RECURSION
	#define MAX_RECURSION 1

	#undef GROUND_TRUTH_REFLECTIONS
	#undef MAX_GT_REFLECTION_SAMPLES
	#define MAX_GT_REFLECTION_SAMPLES 2
	
	//#define NO_SHADOWS
#endif

RaytracingAccelerationStructure Scene : register(t0, space0);

#endif //__DXR_GLOBAL_HLSL__

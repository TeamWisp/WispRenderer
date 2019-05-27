#ifndef __RT_GLOBAL_HLSL__
#define __RT_GLOBAL_HLSL__

#define MAX_RECURSION 3
//#define FOUR_X_A
//#define PATH_TRACING
//#define DEPTH_OF_FIELD
#define EPSILON 5.0
#define SOFT_SHADOWS
#define MAX_SHADOW_SAMPLES 1
#define RUSSIAN_ROULETTE

#ifdef FALLBACK
	#undef MAX_RECURSION
	#define MAX_RECURSION 1

	
	//#define NO_SHADOWS
#endif

RaytracingAccelerationStructure Scene : register(t0, space0);

#ifdef SHADOW_PASS

#ifdef RAY_CONTR_TO_HIT_INDEX
#undef RAY_CONTR_TO_HIT_INDEX
#endif

#ifdef MISS_SHADER_OFFSET
#undef MISS_SHADER_OFFSET
#endif

#define RAY_CONTR_TO_HIT_INDEX 0
#define MISS_SHADER_OFFSET 0

/*
#elif REFLECTION_PASS

#ifdef RAY_CONTR_TO_HIT_INDEX
#undef RAY_CONTR_TO_HIT_INDEX
#endif

#ifdef MISS_SHADER_OFFSET
#undef MISS_SHADER_OFFSET
#endif

#define RAY_CONTR_TO_HIT_INDEX 1
#define MISS_SHADER_OFFSET 1
*/
#endif

#endif //__RT_GLOBAL_HLSL__
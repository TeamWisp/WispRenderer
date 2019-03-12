#define MAX_RECURSION 3
//#define FOUR_X_AA
//#define PATH_TRACING
//#define DEPTH_OF_FIELD
#define EPSILON 0.01
#define SOFT_SHADOWS
#define MAX_SHADOW_SAMPLES 4

#ifdef FALLBACK
	#undef MAX_RECURSION
	#define MAX_RECURSION 1

	#define NO_SHADOWS
#endif

RaytracingAccelerationStructure Scene : register(t0, space0);


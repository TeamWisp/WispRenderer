#define MAX_RECURSION 3
//#define FOUR_X_A
//#define PATH_TRACING
//#define DEPTH_OF_FIELD
#define SOFT_SHADOWS
#define EPSILON 0.02
#define MAX_SHADOW_SAMPLES 1

#ifdef FALLBACK
	#undef MAX_RECURSION
	#define MAX_RECURSION 1

	#define NO_SHADOWS
#endif

//#define HARD
#ifdef HARD
#define RR 1
#define MM 0
#endif

RaytracingAccelerationStructure Scene : register(t0, space0);


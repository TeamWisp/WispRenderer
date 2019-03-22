#define MAX_RECURSION 3
//#define FOUR_X_A
//#define PATH_TRACING
//#define DEPTH_OF_FIELD
#define EPSILON 0.001
#define MAX_SHADOW_SAMPLES 4

#ifdef FALLBACK
	#undef MAX_RECURSION
	#define MAX_RECURSION 1

	#define NO_SHADOWS
#endif


#define HARD_VALUES
#define rr 1
#define mm 0.05

RaytracingAccelerationStructure Scene : register(t0, space0);


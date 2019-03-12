#include "rt_global.hlsl"
#include "util.hlsl"

float DoAmbientOcclusion(float3 normal, float3 wpos, uint spp, float ray_length, uint rand_seed)
{
    float aoValue = 1.0f;
    for(uint i = 0; i< spp; i++)
    {
        aoValue -= (1.0f/float(spp)) * TraceShadowRay(0, wpos + normalize(normal) * EPSILON, getCosHemisphereSample(rand_seed, normal), 0.25f, 1);
    }
    return aoValue;
}
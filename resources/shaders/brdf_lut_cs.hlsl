#include "util.hlsl"
#include "pbr_util.hlsl"

RWTexture2D<float2> output : register(u0);

float2 IntegrateBRDF(float NdotV, float roughness)
{
	float3 V;
	V.x = sqrt(1.0f - NdotV * NdotV);
	V.y = 0.0f;
	V.z = NdotV;

	float A = 0.0f;
	float B = 0.0f;

	float3 N = float3(0.0f, 0.0f, 1.0f);

	const uint SAMPLE_COUNT = 1024u;

	for (uint i = 0; i < SAMPLE_COUNT; ++i)
	{
		float2 Xi = hammersley2d(i, SAMPLE_COUNT);
		float3 H = importanceSample_GGX(Xi, roughness, N);
		float3 L = normalize(2.0f * dot(V, H) * H - V);

		float NdotL = max(L.z, 0.0f);
		float NdotH = max(H.z, 0.0f);
		float VdotH = max(dot(V, H), 0.0f);
		float NdotV = max(dot(N, V), 0.0f);

		if (NdotL > 0.0f)
		{
			float G = GeometrySmith_IBL(NdotV, NdotL, roughness);
			float G_Vis = (G * VdotH) / (NdotH * NdotV);
			float Fc = pow(1.0f - VdotH, 5.0f);

			A += (1.0f - Fc) * G_Vis;
			B += Fc * G_Vis;
		}
	}

	A /= float(SAMPLE_COUNT);
	B /= float(SAMPLE_COUNT);

	return float2(A, B);
}

[numthreads(16, 16, 1)]
void main_cs(uint3 dt_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	output.GetDimensions(screen_size.x, screen_size.y);

	float2 screen_coord = int2(dt_id.x, dt_id.y) + 0.5f;
	float2 uv = screen_coord / screen_size;

	uv.y = 1.0f - uv.y;

	output[dt_id.xy] = IntegrateBRDF(uv.y, uv.x);
}

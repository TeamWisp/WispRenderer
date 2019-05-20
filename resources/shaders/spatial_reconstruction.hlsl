#include "util.hlsl"
#include "pbr_util.hlsl"

cbuffer CameraProperties : register(b0)
{
	float4x4 inv_projection;
	float4x4 inv_view;
	float4x4 view;
};

RWTexture2D<float4> filtered : register(u0);
Texture2D reflection_pdf : register(t0);
Texture2D dir_hitT : register(t1);
Texture2D albedo_roughness : register(t2);
Texture2D normal_metallic : register(t3);
Texture2D depth_buffer : register(t4);
SamplerState nearest_sampler  : register(s0);

float brdf_weight(float3 V, float3 L, float3 N, float roughness)
{
	float3 H = normalize(V + L);

	float NdotH = saturate(dot(N, H));
	float NdotL = saturate(dot(N, L));
	float NdotV = saturate(dot(N, V));

	float G = G_SchlicksmithGGX(NdotL, NdotV, roughness);
	float D = D_GGX(NdotH, roughness);

	return D * G * PI / 4;
}

//Hardcode the samples for now; settings: kernelSize=5, points=16
//https://github.com/Nielsbishere/NoisePlayground/blob/master/bluenoise_test.py

static const uint sampleCount = 16;
static const float2 samples[4][sampleCount] =
	{
		{
			float2(-1 ,  0), float2(0 ,  -2), float2(-2 ,  -2), float2(1 ,  1),
			float2(-1 ,  -3), float2(-3 ,  0), float2(2 ,  -1), float2(-2 ,  2),
			float2(1 ,  2), float2(2 ,  -2), float2(3 ,  -1), float2(2 ,  2),
			float2(-4 ,  -2), float2(-4 ,  2), float2(-3 ,  3), float2(-4 ,  -3)
		},
		{
			float2(0 ,  0), float2(1 ,  -1), float2(-2 ,  1), float2(-3 ,  -1),
			float2(-1 ,  2), float2(0 ,  -3), float2(-4 ,  0), float2(-1 ,  -4),
			float2(-3 ,  -3), float2(1 ,  3), float2(3 ,  1), float2(3 ,  -3),
			float2(-1 ,  -5), float2(4 ,  1), float2(-2 ,  4), float2(-5 ,  1)
		},
		{
			float2(-1 ,  -2), float2(1 ,  0), float2(0 ,  1), float2(-2 ,  -1),
			float2(-2 ,  0), float2(1 ,  -2), float2(0 ,  2), float2(-3 ,  -2),
			float2(-3 ,  1), float2(-3 ,  2), float2(0 ,  3), float2(0 ,  -4),
			float2(3 ,  0), float2(1 ,  -4), float2(3 ,  -2), float2(-2 ,  3)
		},
		{
			float2(-1 ,  -1), float2(0 ,  -1), float2(-1 ,  1), float2(2 ,  0),
			float2(-2 ,  -3), float2(2 ,  1), float2(1 ,  -3), float2(-1 ,  3),
			float2(2 ,  -3), float2(-4 ,  -1), float2(-4 ,  1), float2(-2 ,  -4),
			float2(3 ,  2), float2(-5 ,  0), float2(4 ,  -1), float2(-1 ,  4)
		}
	};

//Sample a neighbor; 0,0 -> 1,1; outside of that range indicates an invalid uv
float2 sample_neighbor_uv(uint sampleId, uint2 fullResPixel, uint2 resolution)
{
	uint pixId = fullResPixel.x % 2 + fullResPixel.y % 2 * 2;
	float2 offset = samples[pixId][sampleId];
	return (float2(fullResPixel / 2) + offset) / float2(resolution / 2 - 1);
}

[numthreads(16, 16, 1)]
void main(int3 pix3 : SV_DispatchThreadID)
{
	//Get dispatch dimensions

	uint2 pix = uint2(pix3.xy);
	uint width, height;
	depth_buffer.GetDimensions(width, height);

	//Get per pixel values

	const float depth = depth_buffer[pix].r;
	const float2 uv = float2(pix.xy) / float2(width - 1, height - 1);
	const float4 ndc = float4(uv * 2 - 1, depth, 1);

	const float4 vpos = mul(inv_projection, ndc);
	const float4 pos4 = mul(inv_view, vpos);
	const float3 pos = (pos4 / pos4.w).xyz;

	const float3 camera_pos = float3(inv_view[0][3], inv_view[1][3], inv_view[2][3]);
	const float3 V = normalize(camera_pos - pos);

	const float roughness = albedo_roughness[pix].w;
	const float3 N = normal_metallic[pix].xyz;

	//Weigh the samples correctly

	float3 result = float3(0, 0, 0);
	float weightSum = 0;

	[unroll]
	for (uint i = 0; i < sampleCount; ++i)
	{
		//Get sample related data

		const float2 neighbor_uv = sample_neighbor_uv(i, pix.xy, uint2(width, height));

		float4 hit_pos = dir_hitT.SampleLevel(nearest_sampler, neighbor_uv, 0);
		hit_pos = float4(hit_pos.xyz * hit_pos.w + pos, 1);
		float4 hit_vpos = mul(view, hit_pos);

		const float3 color = reflection_pdf.SampleLevel(nearest_sampler, neighbor_uv, 0).xyz;
		const float3 L = normalize(hit_vpos.xyz - vpos.xyz);
		const float pdf = max(reflection_pdf.SampleLevel(nearest_sampler, neighbor_uv, 0).w, 1e-5);

		//Calculate weight and weight sum

		const float weight = brdf_weight(V, L, N, roughness) / pdf * float(neighbor_uv.x >= 0 && neighbor_uv.y >= 0 && neighbor_uv.x <= 1 && neighbor_uv.y <= 1);
		result += color * weight;
		weightSum += weight;
	}

	//Output averaged result

	filtered[pix.xy] = float4(result / weightSum, 1);

}
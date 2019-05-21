#include "util.hlsl"
#include "pbr_util.hlsl"

cbuffer CameraProperties : register(b0)
{
	float4x4 inv_projection;
	float4x4 inv_view;
	float4x4 view;

	float2 padding;
	float near_plane, far_plane;
};

RWTexture2D<float4> filtered : register(u0);
Texture2D reflection_pdf : register(t0);
Texture2D dir_hitT : register(t1);
Texture2D albedo_roughness : register(t2);
Texture2D normal_metallic : register(t3);
Texture2D depth_buffer : register(t4);
SamplerState nearest_sampler  : register(s0);

//Get weight from roughness, view direction, light direction and normal (view space)
float brdf_weight(float3 V, float3 L, float3 N, float roughness)
{
	float3 H = normalize(V + L);

	float NdotH = saturate(dot(N, H));
	float NdotL = saturate(dot(N, L));
	float NdotV = saturate(dot(N, V));

	float G = G_SchlicksmithGGX(NdotL, NdotV, roughness);		//This causes issues
	float D = D_GGX(NdotH, roughness);

	float weight = D * PI / 4;

	return max(weight, 1e-5);		//Perfect mirrors can have weights too
}

float linearize_depth(float D)
{
	float z_n = D * 2 - 1;
	return 2 * near_plane * far_plane / (far_plane + near_plane - z_n * (far_plane - near_plane));
}

//Get total weight from neighbor using normal and normalized depth from both neighbor and center
float neighbor_edge_weight(float3 N, float3 N_neighbor, float D, float D_neighbor, float2 uv)
{
	//const float d = linearize_depth(D);
	//const float d_neighbor = linearize_depth(D_neighbor);
	//const float weight = edge_weight(N, N_neighbor, d, d_neighbor);
	return float(uv.x >= 0 && uv.y >= 0 && uv.x <= 1 && uv.y <= 1) * (D != 1 && D_neighbor != 1);
}

//Hardcode the samples for now; settings: kernelSize=3, points=8
//https://github.com/Nielsbishere/NoisePlayground/blob/master/bluenoise_test.py

static const uint sampleCount = 8;
static float2 samples[4][sampleCount] = {
	{
		float2(0 ,  -1), float2(-1 ,  0), float2(1 ,  -1), float2(0 ,  2),
		float2(-3 ,  0), float2(-1 ,  -3), float2(-3 ,  -2), float2(2 ,  1)
	},
	{
		float2(-1 ,  1), float2(0 ,  0), float2(0 ,  -2), float2(-2 ,  -1),
		float2(-2 ,  1), float2(2 ,  -1), float2(1 ,  2), float2(1 ,  -3)
	},
	{
		float2(1 ,  0), float2(0 ,  1), float2(-1 ,  -2), float2(-2 ,  0),
		float2(2 ,  0), float2(-1 ,  2), float2(-3 ,  -1), float2(0 ,  -3)
	},
	{
		float2(-1 ,  -1), float2(-2 ,  -2), float2(1 ,  1), float2(1 ,  -2),
		float2(-3 ,  1), float2(-2 ,  2), float2(2 ,  2), float2(-3 ,  2)
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
	const float3 pos = mul(inv_view, vpos).xyz;

	const float3 camera_pos = float3(inv_view[0][3], inv_view[1][3], inv_view[2][3]);
	const float3 V = normalize(camera_pos - pos);

	const float roughness = max(albedo_roughness[pix].w, 0.05);
	const float3 Nworld = normalize(normal_metallic[pix].xyz);
	const float3 N = mul(view, float4(Nworld, 0)).xyz;

	//Weigh the samples correctly

	float3 result = float3(0, 0, 0);
	float weightSum = 0;

	[unroll]
	for (uint i = 0; i < sampleCount; ++i)
	{
		//Get sample related data

		const float2 neighbor_uv = sample_neighbor_uv(i, pix, uint2(width, height));

		const float4 hitT = dir_hitT.SampleLevel(nearest_sampler, neighbor_uv, 0);
		const float3 hit_pos = hitT.xyz * hitT.w + pos;
		const float3 hit_vpos = mul(view, float4(hit_pos, 1)).xyz;

		const float3 color = reflection_pdf.SampleLevel(nearest_sampler, neighbor_uv, 0).xyz;
		const float3 L = normalize(hit_vpos - vpos.xyz);
		const float pdf = max(reflection_pdf.SampleLevel(nearest_sampler, neighbor_uv, 0).w, 1e-5);
		const float depth_neighbor = depth_buffer.SampleLevel(nearest_sampler, neighbor_uv, 0).r;
		const float3 N_neighbor = normalize(normal_metallic.SampleLevel(nearest_sampler, neighbor_uv, 0).xyz);

		//Calculate weight and weight sum

		const float neighborWeight = neighbor_edge_weight(Nworld, N_neighbor, depth, depth_neighbor, neighbor_uv);
		const float weight = brdf_weight(V, L, N, roughness) / pdf * neighborWeight;
		result += color * weight;
		weightSum += weight;
	}

	//Output averaged result

	float3 result3 = result / weightSum;
	filtered[pix] = float4(result3, 1);

}
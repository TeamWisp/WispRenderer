#ifndef __DENOISING_SPATIAL_RECONSTRUCTION_HLSL__
#define __DENOISING_SPATIAL_RECONSTRUCTION_HLSL__
#include "pbr_util.hlsl"
#include "rand_util.hlsl"

cbuffer CameraProperties : register(b0)
{
	float4x4 inv_vp;
	float4x4 inv_view;

	float padding;
	uint frame_idx;
	float near_plane, far_plane;
};

RWTexture2D<float4> filtered : register(u0);
Texture2D reflection_pdf : register(t0);
Texture2D dir_hitT : register(t1);
Texture2D albedo_roughness : register(t2);
Texture2D normal_metallic : register(t3);
Texture2D depth_buffer : register(t4);
SamplerState nearest_sampler  : register(s0);

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

static const uint sample_count = 16;
static const float2 samples[4][64] = {
    {
        float2(-1 ,  -1), float2(0 ,  -1), float2(1 ,  1), float2(0 ,  2),
        float2(-3 ,  -1), float2(-2 ,  2), float2(2 ,  -2), float2(3 ,  0),
        float2(2 ,  -3), float2(-1 ,  -4), float2(1 ,  3), float2(3 ,  1),
        float2(-2 ,  -4), float2(-4 ,  1), float2(-3 ,  3), float2(-1 ,  4),
        float2(4 ,  -1), float2(-5 ,  -2), float2(3 ,  -4), float2(-4 ,  -4),
        float2(-5 ,  -3), float2(0 ,  -6), float2(-6 ,  0), float2(-4 ,  4),
        float2(5 ,  1), float2(1 ,  5), float2(1 ,  -6), float2(-3 ,  5),
        float2(2 ,  5), float2(5 ,  -3), float2(6 ,  -1), float2(-1 ,  6),
        float2(-6 ,  3), float2(-4 ,  -6), float2(5 ,  3), float2(-7 ,  -2),
        float2(-7 ,  1), float2(-2 ,  -7), float2(6 ,  -3), float2(5 ,  -5),
        float2(5 ,  4), float2(-7 ,  -4), float2(3 ,  -7), float2(3 ,  6),
        float2(6 ,  3), float2(0 ,  -8), float2(-1 ,  7), float2(7 ,  -2),
        float2(5 ,  5), float2(-5 ,  -7), float2(3 ,  -8), float2(-8 ,  3),
        float2(-4 ,  7), float2(-9 ,  -1), float2(-6 ,  6), float2(8 ,  0),
        float2(1 ,  8), float2(1 ,  -9), float2(4 ,  7), float2(-8 ,  -5),
        float2(-3 ,  -9), float2(-9 ,  -3), float2(2 ,  8), float2(8 ,  3)

    },
    {
        float2(1 ,  0), float2(-1 ,  -2), float2(-2 ,  -1), float2(-1 ,  1),
        float2(2 ,  -1), float2(-1 ,  -3), float2(1 ,  2), float2(-3 ,  2),
        float2(-3 ,  -3), float2(-4 ,  -1), float2(-1 ,  3), float2(-4 ,  0),
        float2(1 ,  -4), float2(-2 ,  3), float2(3 ,  2), float2(3 ,  -3),
        float2(-4 ,  -3), float2(4 ,  -2), float2(-2 ,  -5), float2(3 ,  3),
        float2(-5 ,  2), float2(2 ,  -5), float2(5 ,  -1), float2(-6 ,  2),
        float2(-3 ,  -6), float2(5 ,  2), float2(-5 ,  -5), float2(3 ,  -6),
        float2(3 ,  5), float2(0 ,  6), float2(0 ,  -7), float2(1 ,  -7),
        float2(1 ,  6), float2(-2 ,  6), float2(2 ,  6), float2(-7 ,  -3),
        float2(6 ,  2), float2(-7 ,  2), float2(-6 ,  -5), float2(4 ,  -6),
        float2(-5 ,  5), float2(-4 ,  -7), float2(-8 ,  0), float2(-1 ,  -8),
        float2(7 ,  0), float2(-8 ,  1), float2(-8 ,  -2), float2(7 ,  -3),
        float2(-8 ,  -3), float2(-5 ,  6), float2(-3 ,  7), float2(7 ,  -4),
        float2(-4 ,  -8), float2(-7 ,  5), float2(0 ,  -9), float2(5 ,  -7),
        float2(-7 ,  -6), float2(6 ,  -6), float2(8 ,  1), float2(-2 ,  8),
        float2(-9 ,  1), float2(-2 ,  -9), float2(7 ,  4), float2(-8 ,  4)

    },
    {
        float2(-1 ,  0), float2(0 ,  0), float2(1 ,  -1), float2(1 ,  -2),
        float2(-3 ,  0), float2(0 ,  -3), float2(-3 ,  1), float2(-3 ,  -2),
        float2(0 ,  -4), float2(0 ,  3), float2(2 ,  2), float2(3 ,  -2),
        float2(-3 ,  -4), float2(-4 ,  2), float2(2 ,  3), float2(4 ,  0),
        float2(-5 ,  -1), float2(-5 ,  0), float2(0 ,  4), float2(1 ,  -5),
        float2(4 ,  1), float2(-2 ,  4), float2(-3 ,  -5), float2(4 ,  -3),
        float2(-1 ,  -6), float2(-2 ,  5), float2(3 ,  -5), float2(-5 ,  -4),
        float2(4 ,  3), float2(-5 ,  3), float2(4 ,  -4), float2(3 ,  4),
        float2(-4 ,  5), float2(-6 ,  -4), float2(-7 ,  0), float2(6 ,  0),
        float2(-7 ,  -1), float2(-1 ,  -7), float2(6 ,  -2), float2(-6 ,  4),
        float2(-5 ,  -6), float2(4 ,  5), float2(-4 ,  6), float2(6 ,  -4),
        float2(0 ,  7), float2(7 ,  1), float2(-6 ,  5), float2(-3 ,  -8),
        float2(2 ,  7), float2(2 ,  -8), float2(6 ,  -5), float2(4 ,  -7),
        float2(-8 ,  2), float2(7 ,  3), float2(3 ,  7), float2(6 ,  5),
        float2(5 ,  6), float2(8 ,  -1), float2(0 ,  8), float2(8 ,  -2),
        float2(-5 ,  7), float2(4 ,  -8), float2(7 ,  -5), float2(6 ,  6)

    },
    {
        float2(0 ,  -2), float2(0 ,  1), float2(-2 ,  0), float2(-2 ,  -2),
        float2(-2 ,  1), float2(2 ,  0), float2(-1 ,  2), float2(-2 ,  -3),
        float2(1 ,  -3), float2(2 ,  1), float2(3 ,  -1), float2(-4 ,  -2),
        float2(2 ,  -4), float2(0 ,  -5), float2(-1 ,  -5), float2(-5 ,  1),
        float2(1 ,  4), float2(-4 ,  3), float2(2 ,  4), float2(-3 ,  4),
        float2(4 ,  2), float2(-6 ,  -1), float2(0 ,  5), float2(5 ,  0),
        float2(-1 ,  5), float2(5 ,  -2), float2(-4 ,  -5), float2(-6 ,  1),
        float2(-6 ,  -2), float2(-2 ,  -6), float2(2 ,  -6), float2(-6 ,  -3),
        float2(-5 ,  4), float2(4 ,  -5), float2(4 ,  4), float2(5 ,  -4),
        float2(6 ,  1), float2(2 ,  -7), float2(-3 ,  6), float2(-3 ,  -7),
        float2(-7 ,  3), float2(7 ,  -1), float2(-8 ,  -1), float2(-2 ,  7),
        float2(1 ,  7), float2(1 ,  -8), float2(-2 ,  -8), float2(5 ,  -6),
        float2(-6 ,  -6), float2(4 ,  6), float2(-7 ,  4), float2(6 ,  4),
        float2(7 ,  2), float2(-7 ,  -5), float2(-8 ,  -4), float2(-9 ,  0),
        float2(-1 ,  -9), float2(-1 ,  8), float2(-6 ,  -7), float2(-9 ,  -2),
        float2(-5 ,  -8), float2(-9 ,  2), float2(-3 ,  8), float2(8 ,  -3)

    }
};

//Sample a neighbor; 0,0 -> 1,1; outside of that range indicates an invalid uv
float2 sample_neighbor_uv(uint sampleId, uint2 full_res_pixel, uint2 resolution, float rand, float kernelSize)
{
	uint pixId = full_res_pixel.x % 2 + full_res_pixel.y % 2 * 2;
	float2 offset = samples[pixId][64 * rand] * kernelSize;
	return (float2(full_res_pixel / 2) + offset) / float2(resolution / 2 - 1);
}

static const float3 luminance = float3(0.2126f, 0.7152f, 0.0722f);

float3 unpack_position(float2 uv, float depth)
{
  // Get world space position
  const float4 ndc = float4(uv * 2.0 - 1.0, depth, 1.0);
  float4 wpos = mul(inv_vp, ndc);
  return (wpos.xyz / wpos.w).xyz;
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
	const float3 pos = unpack_position(uv, depth);

	const float3 camera_pos = float3(inv_view[0][3], inv_view[1][3], inv_view[2][3]);
	const float3 V = normalize(camera_pos - pos);

	const float roughness = max(albedo_roughness[pix].w, 0.001);
	const float3 N = normalize(normal_metallic[pix].xyz);

	const float pdf = max(reflection_pdf.SampleLevel(nearest_sampler, uv, 0).w, 0);

	//Weigh the samples correctly

	float3 result = float3(0, 0, 0);
	float weight_sum = 0;

	uint rand_seed = initRand(pix.x + pix.y * width, frame_idx);

	float distance = length(camera_pos - pos);
	float sampleCountScalar = (1 - distance / far_plane) * roughness;

	float kernel_size = 16 * sampleCountScalar;

	for (uint i = 0; i < ceil(kernel_size); ++i)
	{
		//Get sample related data

		float randomVar = nextRand(rand_seed);

		const float2 neighbor_uv = sample_neighbor_uv(i, pix, uint2(width, height), randomVar, sampleCountScalar);

		const float depth_neighbor = depth_buffer.SampleLevel(nearest_sampler, neighbor_uv, 0).r;
		const float3 pos_neighbor = unpack_position(neighbor_uv, depth_neighbor);

		const float4 hitT = dir_hitT.SampleLevel(nearest_sampler, neighbor_uv, 0);
        //const float3 hit_pos = hitT.xyz * hitT.w + pos_neighbor;

		const float3 V_neighbor = normalize(camera_pos - pos_neighbor);

		const float3 color = clamp(reflection_pdf.SampleLevel(nearest_sampler, neighbor_uv, 0).xyz, 0, 1);
		const float3 L = hitT.xyz;
		const float pdf_neighbor = max(reflection_pdf.SampleLevel(nearest_sampler, neighbor_uv, 0).w, 1e-5);
		const float3 N_neighbor = normalize(normal_metallic.SampleLevel(nearest_sampler, neighbor_uv, 0).xyz);

		//Calculate weight and weight sum

		const float neighbor_weight = neighbor_edge_weight(N, N_neighbor, depth, depth_neighbor, neighbor_uv);
		const float weight = brdf_weight(V, L, N, roughness) / pdf * neighbor_weight;
		result += color * weight;
		weight_sum += weight;
	}

	//Output averaged result

	float3 result3 = result / weight_sum;
	//float3 result3 = float3((pdf > 1).xxx);
	//result3 /= 1 + dot(luminance, result3);
	//float3 result3 = reflection_pdf.SampleLevel(nearest_sampler, uv, 0).xyz;
	filtered[pix] = float4(result3, 1);

}

#endif
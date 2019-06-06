#include "util.hlsl"
#include "pbr_util.hlsl"

Texture2D input_texture : register(t0);
Texture2D dir_hitT_texture : register(t1);
Texture2D albedo_roughness_texture : register(t2);
Texture2D normal_metallic_texture : register(t3);
Texture2D motion_texture : register(t4); // xy: motion, z: fwidth position, w: fwidth normal
Texture2D linear_depth_texture : register(t5);
Texture2D world_position_texture : register(t6);
RWTexture2D<float4> output_texture : register(u0);

SamplerState point_sampler : register(s0);
SamplerState linear_sampler : register(s0);

cbuffer CameraProperties : register(b0)
{
	float4x4 view;
	float4x4 projection;
	float4x4 inv_projection;
	float4x4 inv_view;
	float4x4 prev_projection;
	float4x4 prev_view;
	uint is_hybrid;
	uint is_path_tracer;
	uint is_ao;
	uint has_shadows;
	uint has_reflections;
	float3 padding;
};

float3 unpack_position(float2 uv, float depth, float4x4 proj_inv, float4x4 view_inv) {
	const float4 ndc = float4(uv * 2.0f - 1.f, depth, 1.0f);
	const float4 pos = mul( view_inv, mul(proj_inv, ndc));
	return (pos / pos.w).xyz;
}

float3 unpack_position_linear_depth(float2 uv, float depth, float4x4 proj_inv, float4x4 view_inv) {
	const float4 ndc = float4(uv * 2.0f - 1.f, 1.f, 1.0f);
	const float4 pos = mul( view_inv, mul(proj_inv, ndc));
	return normalize((pos / pos.w).xyz) * depth;
}

float3 OctToDir(uint octo)
{
	float2 e = float2( f16tof32(octo & 0xFFFF), f16tof32((octo>>16) & 0xFFFF) ); 
	float3 v = float3(e, 1.0 - abs(e.x) - abs(e.y));
	if (v.z < 0.0)
		v.xy = (1.0 - abs(v.yx)) * (step(0.0, v.xy)*2.0 - (float2)(1.0));
	return normalize(v);
}

float Luminance(float3 color)
{
	return (color.r + color.r + color.b + color.g + color.g + color.g) / 6.0;
}

uint DirToOct(float3 normal)
{
	float2 p = normal.xy * (1.0 / dot(abs(normal), 1.0.xxx));
	float2 e = normal.z > 0.0 ? p : (1.0 - abs(p.yx)) * (step(0.0, p)*2.0 - (float2)(1.0));
	return (asuint(f32tof16(e.y)) << 16) + (asuint(f32tof16(e.x)));
}

void FetchNormalAndLinearZ(in int2 ipos, out float3 norm, out float2 zLinear)
{
	norm = normal_metallic_texture[ipos].xyz;
	zLinear = linear_depth_texture[ipos].xy;
}

float NormalDistanceCos(float3 n1, float3 n2, float power)
{
	return pow(max(0.0, dot(n1, n2)), power);
}

float ComputeWeightNoLuminance(float depth_center, float depth_p, float phi_depth, float3 normal_center, float3 normal_p)
{
	const float w_normal    = NormalDistanceCos(normal_center, normal_p, 128.f);
	const float w_z         = abs(depth_center - depth_p) / phi_depth;

	return exp(-max(w_z, 0.0)) * w_normal;
}

float ComputeWeight(
	float depth_center, float depth_p, float phi_depth,
	float3 normal_center, float3 normal_p, float norm_power, 
	float luminance_direct_center, float luminance_direct_p, float phi_direct)
{
	const float w_normal    = NormalDistanceCos(normal_center, normal_p, norm_power);
	const float w_z         = (phi_depth == 0) ? 0.0f : abs(depth_center - depth_p) / phi_depth;
	const float w_l_direct   = abs(luminance_direct_center - luminance_direct_p) / phi_direct;

	const float w_direct   = exp(0.0 - max(w_l_direct, 0.0)   - max(w_z, 0.0)) * w_normal;

	return w_direct;
}

[numthreads(16, 16, 1)]
void spatial_denoiser_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
    float2 screen_coord = float2(dispatch_thread_id.xy);

    float2 screen_size = float2(0, 0);
    output_texture.GetDimensions(screen_size.x, screen_size.y);

    float2 uv = screen_coord / screen_size;

    const float depth = linear_depth_texture[screen_coord].x;

    float3 world_pos = world_position_texture[screen_coord].xyz;
    float3 center_pos = world_position_texture[screen_size / 2].xyz;
    float3 cam_pos = float3(inv_view[0][3], inv_view[1][3], inv_view[2][3]);

    float3 V = normalize(cam_pos - world_pos);

    float3 center_V = normalize(cam_pos - center_pos);
    float3 N = normalize(normal_metallic_texture[screen_size / 2].xyz);

    float3 L = reflect(-center_V, N);

    float roughness = albedo_roughness_texture[screen_size / 2].xyz;

    output_texture[screen_coord] = float4(brdf_weight(V, L, N, roughness).xxxx) + input_texture[screen_coord];
}
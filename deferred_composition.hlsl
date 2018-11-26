#include "fullscreen_quad.hlsl"
#include "pbr_util.hlsl"

Texture2D gbuffer_albedo : register(t0);
Texture2D gbuffer_normal : register(t1);
Texture2D gbuffer_depth : register(t2);
SamplerState s0 : register(s0);

cbuffer CameraProperties : register(b0)
{
	float4x4 view;
	float4x4 projection;
	float4x4 inv_projection;
};

static uint min_depth = 0xFFFFFFFF;
static uint max_depth = 0x0;

float3 unpack_position(float2 uv, float depth, float4x4 proj_inv) {
	const float4 ndc = float4(uv * 2.0 - 1.0, depth, 1.0);
	const float4 vpos = mul(proj_inv, ndc);
	return (vpos / vpos.w).xyz;
}

float3 world_to_view(float4 pos, float4x4 view) {
	const float4 vpos = mul(view, pos);
	return vpos.xyz;
}

float4 main_ps(VS_OUTPUT input) : SV_TARGET
{
	const float2 uv = input.uv;

	// GBuffer contents
	const float3 albedo = gbuffer_albedo.Sample(s0, uv).xyz;
	const float3 normal = gbuffer_normal.Sample(s0, uv).xyz;
	const float depth_f = gbuffer_depth.Sample(s0, uv).r;

	// View position and camera position
	float3 vpos = unpack_position(float2(uv.x, 1.0 - uv.y), depth_f, inv_projection);
	float3 V = normalize(-vpos);

	// Lighting
	float ambient = 0.1f;
	float3 lighting = float3(ambient, ambient, ambient);
	// float3 L = float3(0, 0, 1);
	float3 L = world_to_view(float4(normalize(float3(0, 0, -1)), 0), view);
	float3 light_color = float3(1, 1, 1);
	float3 radiance = light_color * 3;
	lighting += BRDF(L, V, normal, 0.4f, 0.6f, albedo, radiance, light_color);

	float3 color = albedo * lighting;

	return float4(color, 1.f);
}

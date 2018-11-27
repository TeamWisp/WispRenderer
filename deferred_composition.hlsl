#include "fullscreen_quad.hlsl"
#include "pbr_util.hlsl"

struct Light 
{
	float3 pos;			//Position in world space for spot & point
	float rad;			//Radius for point, height for spot

	float3 col;			//Color
	uint tid;			//Type id; light_type_x

	float3 dir;			//Direction for spot & directional
	float ang;			//Angle for spot; in radians
};

StructuredBuffer<Light> lights : register(t3);

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

static uint light_type_point = 0;
static uint light_type_directional = 1;
static uint light_type_spot = 2;

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

float3 shade(float3 vpos, float3 V, float3 albedo, float3 normal, Light light) 
{
	uint tid = light.tid & 3;

	//Light direction (constant with directional, position dependent with other)
	float3 light_dir = lerp(light.pos - vpos, -light.dir, tid == light_type_directional);
	float light_dist = length(light_dir);
	light_dir /= light_dist;

	//Spot intensity (only used with spot; but always calculated)
	float min_cos = cos(light.ang);
	float max_cos = lerp(min_cos, 1, 0.5f);
	float cos_angle = dot(light.dir, -light_dir);
	float spot_intensity = lerp(smoothstep(min_cos, max_cos, cos_angle), 1, tid != light_type_spot);

	//Attenuation & spot intensity (only used with point or spot)
	float intensity = spot_intensity * lerp(1.0f - smoothstep(0, light.rad, light_dist), 1, tid == light_type_directional);

	//Light calculation
	float3 L = world_to_view(float4(light_dir, 0), view);
	float3 light_color = light.col;

	float3 radiance = light_color * 3;
	float3 lighting = BRDF(L, V, normal, 0.4f, 0.6f, albedo, radiance, light_color) * intensity;

	return lighting;

}

float3 shade(float3 vpos, float3 V, float3 albedo, float3 normal)
{

	uint light_count = lights[0].tid >> 2;	//Light count is stored in 30 upper-bits of first light

	float ambient = 0.1f;
	float3 res = float3(ambient, ambient, ambient);

	for (uint i = 0; i < light_count; i++)
	{
		res += shade(vpos, V, albedo, normal, lights[i]);
	}

	return res * albedo;

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

	//Do shading
	return float4(shade(vpos, V, albedo, normal), 1.f);
}
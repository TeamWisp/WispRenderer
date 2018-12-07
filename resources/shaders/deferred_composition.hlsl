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
RWTexture2D<float4> output : register(u0);
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

float3 shade_light(float3 vpos, float3 V, float3 albedo, float3 normal, Light light) 
{
	uint tid = light.tid & 3;

	// Light pos and view directon to view.
	float3 light_vpos = mul(view, float4(light.pos, 1)).xyz;
	float3 light_vdir = normalize(mul(view, float4(light.dir, 0))).xyz;

	//Light direction (constant with directional, position dependent with other)
	float3 L = (lerp(light_vpos - vpos, light.dir, tid == light_type_directional));
	float light_dist = length(L);
	L /= light_dist;

	//Spot intensity (only used with spot; but always calculated)
	float min_cos = cos(light.ang);
	float max_cos = lerp(min_cos, 1, 0.5f);
	float cos_angle = dot(light.dir, -L);
	float spot_intensity = lerp(smoothstep(min_cos, max_cos, cos_angle), 1, tid != light_type_spot);

	//Attenuation & spot intensity (only used with point or spot)
	float attenuation = lerp(1.0f - smoothstep(0, light.rad, light_dist), 1, tid == light_type_directional);
	float3 radiance = (light.col * spot_intensity) * attenuation;

	float3 lighting = BRDF(L, V, normal, 0.001f, 0.999f, albedo, radiance, light.col);

	return lighting;

}

float3 shade_pixel(float3 vpos, float3 V, float3 albedo, float3 normal)
{

	uint light_count = lights[0].tid >> 2;	//Light count is stored in 30 upper-bits of first light

	float ambient = 0.1f;
	float3 res = float3(ambient, ambient, ambient);

	for (uint i = 0; i < light_count; i++)
	{
		res += shade_light(vpos, V, albedo, normal, lights[i]);
	}

	return res * albedo;

}

[numthreads(16, 16, 1)]
void main_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	output.GetDimensions(screen_size.x, screen_size.y);
	float2 uv = float2(dispatch_thread_id.x / screen_size.x, 1.f - (dispatch_thread_id.y / screen_size.y));

	float2 screen_coord = int2(dispatch_thread_id.x, screen_size.y - dispatch_thread_id.y);

	// GBuffer contents
	const float3 albedo = gbuffer_albedo[screen_coord].xyz;
	const float3 normal = gbuffer_normal[screen_coord].xyz;
	const float depth_f = gbuffer_depth[screen_coord].r;

	// View position and camera position
	float3 vpos = unpack_position(float2(uv.x, 1.f - uv.y), depth_f, inv_projection);
	float3 V = normalize(-vpos);

	//Do shading
	output[int2(dispatch_thread_id.xy)] = float4(shade_pixel(vpos, V, albedo, normal), 1.f);
}

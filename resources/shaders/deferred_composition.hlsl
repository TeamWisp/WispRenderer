#define LIGHTS_REGISTER register(t4)
#define MAX_REFLECTION_LOD 4

#include "fullscreen_quad.hlsl"
#include "util.hlsl"
#include "pbr_util.hlsl"
#include "hdr_util.hlsl"
#include "lighting.hlsl"

Texture2D gbuffer_albedo_roughness : register(t0);
Texture2D gbuffer_normal_metallic : register(t1);
Texture2D gbuffer_emissive_ao : register(t2);
Texture2D gbuffer_depth : register(t3);
//Consider SRV for light buffer in register t4
TextureCube skybox : register(t5);
TextureCube irradiance_map   : register(t6);
TextureCube pref_env_map	 : register(t7);
Texture2D brdf_lut			 : register(t8);
Texture2D buffer_refl_shadow : register(t9); // xyz: reflection, a: shadow factor
Texture2D screen_space_irradiance : register(t10);
Texture2D screen_space_ao : register(t11);
RWTexture2D<float4> output   : register(u0);
SamplerState point_sampler   : register(s0);
SamplerState linear_sampler  : register(s1);

cbuffer CameraProperties : register(b0)
{
	float4x4 view;
	float4x4 projection;
	float4x4 inv_projection;
	float4x4 inv_view;
	uint is_hybrid;
	uint is_path_tracer;
	uint is_hbao;
};

static uint min_depth = 0xFFFFFFFF;
static uint max_depth = 0x0;

float3 unpack_position(float2 uv, float depth, float4x4 proj_inv, float4x4 view_inv) {
	const float4 ndc = float4(uv * 2.0f - 1.f, depth, 1.0);
	const float4 pos = mul( view_inv, mul(proj_inv, ndc));
	return (pos / pos.w).xyz;
}

[numthreads(16, 16, 1)]
void main_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	output.GetDimensions(screen_size.x, screen_size.y);

	// added offset of 0.5f to select proper pixel to sample from.
	// screen coords always get floored, therefore if the coords would be (1.9, 1.0),
	// it would sample (1.0, 1.0) instead of the intended (2.0, 1.0). Adding the offset solves this problem.
	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y) + 0.5f;

	float2 uv = screen_coord / screen_size;

	const float depth_f = gbuffer_depth.SampleLevel(point_sampler, uv, 0).r;

	// View position and camera position
	float3 pos = unpack_position(float2(uv.x, 1.f - uv.y), depth_f, inv_projection, inv_view);
	float3 camera_pos = float3(inv_view[0][3], inv_view[1][3], inv_view[2][3]);
	float3 V = normalize(camera_pos - pos);
	
	float3 retval;
	
	if(depth_f != 1.0f)
	{
		// GBuffer contents
		float4 albedo_roughness = gbuffer_albedo_roughness.SampleLevel(point_sampler, uv, 0);
		float3 albedo = albedo_roughness.xyz;
		const float roughness = albedo_roughness.w;

		float4 normal_metallic = gbuffer_normal_metallic.SampleLevel(point_sampler, uv, 0);
		float3 normal = normalize(normal_metallic.xyz);
		const float metallic = normal_metallic.w;

		float4 emissive_ao = gbuffer_emissive_ao.SampleLevel(point_sampler, uv, 0);
		float3 emissive = emissive_ao.xyz;
		float gbuffer_ao = emissive_ao.w;

		float3 flipped_N = normal;
		flipped_N.y *= -1;
		
		const float2 sampled_brdf = brdf_lut.SampleLevel(point_sampler, float2(max(dot(normal, V), 0.01f), roughness), 0).rg;
		float3 sampled_environment_map = pref_env_map.SampleLevel(linear_sampler, reflect(-V, normal), roughness * MAX_REFLECTION_LOD);
		
		// Get irradiance
		float3 irradiance = lerp(
			irradiance_map.SampleLevel(linear_sampler, flipped_N, 0).xyz,
			screen_space_irradiance.SampleLevel(point_sampler, uv, 0).xyz,
			is_path_tracer);

		// Get ao
		float ao = lerp(
			1,
			screen_space_ao.SampleLevel(point_sampler, uv, 0).xyz,
			// Lerp factor (0: env map, 1: path traced)
			is_hbao);

		//Ao is multiplied with material texture ao, if present
		ao *= gbuffer_ao;

		// Get shadow factor (0: fully shadowed, 1: no shadow)
		float shadow_factor = lerp(
			// Do deferred shadow (fully lit for now)
			1.0,
			// Shadow buffer if its hybrid rendering
			buffer_refl_shadow.SampleLevel(point_sampler, uv, 0).a,
			// Lerp factor (0: no hybrid, 1: hybrid)
			is_hybrid);

		shadow_factor = clamp(shadow_factor, 0.0, 1.0);
		
		// Get reflection
		float3 reflection = lerp(
			// Sample from environment if it IS NOT hybrid rendering
			sampled_environment_map,
			// Reflection buffer if it IS hybrid rendering
			buffer_refl_shadow.SampleLevel(point_sampler, uv, 0).xyz,
			// Lerp factor (0: no hybrid, 1: hybrid)
			is_hybrid);

		// Shade pixel
		retval = shade_pixel(pos, V, albedo, metallic, roughness, emissive, normal, irradiance, ao, reflection, sampled_brdf, shadow_factor);
	}
	else
	{	
		retval = skybox.SampleLevel(linear_sampler, -V, 0);
	}

	//Do shading
	output[int2(dispatch_thread_id.xy)] = float4(retval, 1.f);
}

#define LIGHTS_REGISTER register(t3)

#include "fullscreen_quad.hlsl"
#include "util.hlsl"
#include "pbr_util.hlsl"
#include "hdr_util.hlsl"
#include "lighting.hlsl"

Texture2D gbuffer_albedo_roughness : register(t0);
Texture2D gbuffer_normal_metallic : register(t1);
Texture2D gbuffer_depth : register(t2);
//Consider SRV for light buffer in register t3
Texture2D skybox : register(t4);
TextureCube irradiance_map : register(t5);
RWTexture2D<float4> output : register(u0);
SamplerState s0 : register(s0);

cbuffer CameraProperties : register(b0)
{
	float4x4 view;
	float4x4 projection;
	float4x4 inv_projection;
	float4x4 inv_view;
};

static uint min_depth = 0xFFFFFFFF;
static uint max_depth = 0x0;

float3 unpack_position(float2 uv, float depth, float4x4 proj_inv, float4x4 view_inv) {
	const float4 ndc = float4(uv * 2.0 - 1.0, depth, 1.0);
	const float4 pos = mul( view_inv, mul(proj_inv, ndc));
	return (pos / pos.w).xyz;
}

[numthreads(16, 16, 1)]
void main_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
	float2 screen_size = float2(0.f, 0.f);
	output.GetDimensions(screen_size.x, screen_size.y);
	float2 uv = float2(dispatch_thread_id.x / screen_size.x, dispatch_thread_id.y / screen_size.y);

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y);

	const float depth_f = gbuffer_depth[screen_coord].r;

	// View position and camera position
	float3 pos = unpack_position(float2(uv.x, 1.f - uv.y), depth_f, inv_projection, inv_view);
	float3 camera_pos = float3(inv_view[0][3], inv_view[1][3], inv_view[2][3]);
	float3 V = normalize(camera_pos - pos);

	float3 retval;
	
	if(depth_f != 1.0f)
	{
		// GBuffer contents
		float3 albedo = gbuffer_albedo_roughness[screen_coord].xyz;
		const float roughness = gbuffer_albedo_roughness[screen_coord].w;
		float3 normal = gbuffer_normal_metallic[screen_coord].xyz;
		const float metallic = gbuffer_normal_metallic[screen_coord].w;

		float3 flipped_N = normal;
		flipped_N.y *= -1;
		const float3 sampled_irradiance = irradiance_map.SampleLevel(s0, flipped_N, 0).xyz;

		const float shadow_factor = 1.0f;
		
		float3 skybox_reflection = skybox.SampleLevel(s0, SampleSphericalMap(reflect(-V, normal)), 0);

		retval = shade_pixel(pos, V, albedo, metallic, roughness, normal, sampled_irradiance, skybox_reflection);

		retval = retval * shadow_factor;
	}
	else
	{	
		retval = skybox.SampleLevel(s0, SampleSphericalMap(-V), 0);
	}

	//Do shading
	output[int2(dispatch_thread_id.xy)] = float4(retval, 1.f);
}

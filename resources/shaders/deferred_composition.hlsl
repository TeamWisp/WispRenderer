#define LIGHTS_REGISTER register(t3)
#define MAX_REFLECTION_LOD 4

#include "fullscreen_quad.hlsl"
#include "util.hlsl"
#include "pbr_util.hlsl"
#include "hdr_util.hlsl"
#include "lighting.hlsl"

Texture2D gbuffer_albedo_roughness : register(t0);
Texture2D gbuffer_normal_metallic : register(t1);
Texture2D gbuffer_depth : register(t2);
//Consider SRV for light buffer in register t3
TextureCube skybox : register(t4);
TextureCube irradiance_map   : register(t5);
TextureCube pref_env_map	 : register(t6);
Texture2D brdf_lut			 : register(t7);
Texture2D buffer_refl_shadow : register(t8); // xyz: reflection, a: shadow factor
Texture2D screen_space_irradiance : register(t9);
Texture2D screen_space_ao : register(t10);
RWTexture2D<float4> output   : register(u0);
RWStructuredBuffer<Surfel> g_surfels   : register(u1);
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
		float roughness = gbuffer_albedo_roughness[screen_coord].w;
		float3 normal = gbuffer_normal_metallic[screen_coord].xyz;
		float metallic = gbuffer_normal_metallic[screen_coord].w;
		#ifdef HARD
		roughness = RR;
		metallic = MM;
		#endif

		float3 flipped_N = normal;
		flipped_N.y *= -1;
		
		const float2 sampled_brdf = brdf_lut.SampleLevel(point_sampler, float2(max(dot(normal, V), 0.01f), roughness), 0).rg;
		float3 sampled_environment_map = pref_env_map.SampleLevel(linear_sampler, reflect(-V, normal), roughness * MAX_REFLECTION_LOD);
		
		// Get irradiance
		float3 irradiance = lerp(
			irradiance_map.SampleLevel(linear_sampler, flipped_N, 0).xyz,
			screen_space_irradiance[screen_coord].xyz,
			// Lerp factor (0: env map, 1: path traced)
			is_path_tracer);

		// Get ao
		float ao = lerp(
			1,
			screen_space_ao[screen_coord].xyz,
			// Lerp factor (0: env map, 1: path traced)
			true);

		// Get shadow factor (0: fully shadowed, 1: no shadow)
		float shadow_factor = lerp(
			// Do deferred shadow (fully lit for now)
			1.0,
			// Shadow buffer if its hybrid rendering
			buffer_refl_shadow[screen_coord].a,
			// Lerp factor (0: no hybrid, 1: hybrid)
			is_hybrid);

		shadow_factor = clamp(shadow_factor, 0.0, 1.0);
		
		// Get reflection
		float3 reflection = lerp(
			// Sample from environment if it IS NOT hybrid rendering
			sampled_environment_map,
			// Reflection buffer if it IS hybrid rendering
			buffer_refl_shadow[screen_coord].xyz,	
			// Lerp factor (0: no hybrid, 1: hybrid)
			is_hybrid);
	
		/*if (is_path_tracer)
		{
			Surfel closest_surfel_a = g_surfels[2];
			Surfel closest_surfel_b = g_surfels[1];
			Surfel closest_surfel_c = g_surfels[2];
			float closest_dist_a = 999999999999;
			float closest_dist_b = 999999999999;
			float closest_dist_c = 999999999999;
			for (int i = 0; i < 3000; i++)
			{
				float dist = length(g_surfels[i].position - pos);

				if (dist < closest_dist_a)
				{
					closest_surfel_a = g_surfels[i];
					closest_dist_a = dist;
					//continue;
				}

				else if (dist < closest_dist_b)
				{
					closest_surfel_b = g_surfels[i];
					closest_dist_b = dist;
					//continue;
				}

				else if (dist < closest_dist_c)
				{
					closest_surfel_c = g_surfels[i];
					closest_dist_c = dist;
					//continue;
				}
			}

			float3 f = pos;
			// calculate vectors from point f to vertices p1, p2 and p3:
			float3 f1 = closest_surfel_a.position.rgb - f;
			float3 f2 = closest_surfel_b.position.rgb - f;
			float3 f3 = closest_surfel_c.position.rgb - f;
			// calculate the areas (parameters order is essential in this case):
			float3 va = cross(closest_surfel_a.position - closest_surfel_b.position, closest_surfel_a.position - closest_surfel_c.position); // main triangle cross product
			float3 va1 = cross(f2, f3); // p1's triangle cross product
			float3 va2 = cross(f3, f1); // p2's triangle cross product
			float3 va3 = cross(f1, f2); // p3's triangle cross product
			float a = length(va); // main triangle area
			// calculate barycentric coordinates with sign:
			float a1 = length(va1) / a * sign(dot(va, va1));
			float a2 = length(va2) / a * sign(dot(va, va2));
			float a3 = length(va3) / a * sign(dot(va, va3));

			float3 bar_color = closest_surfel_a.color.rgb * a1 + closest_surfel_b.color.rgb * a2 + closest_surfel_c.color.rgb * a3;

			float w1 = 1.f / length(closest_surfel_a.position.rgb - f);
			float w2 = 1.f / length(closest_surfel_b.position.rgb - f);
			float w3 = 1.f / length(closest_surfel_c.position.rgb - f);

			//irradiance = (a1 * closest_surfel_a.color.rgb + a2 * closest_surfel_b.color.rgb + a3 * closest_surfel_c.color.rgb) / (a1 + a2 + a3);
			//irradiance = (w1 * closest_surfel_a.color.rgb + w2 * closest_surfel_b.color.rgb + w3 * closest_surfel_c.color.rgb) / (w1 + w2 + w3);
			irradiance = closest_surfel_a.color.rgb;

		}*/

		// Shade pixel
		retval = shade_pixel(pos, V, albedo, metallic, roughness, normal, irradiance, ao, reflection, sampled_brdf, shadow_factor);
		retval = irradiance;
	}
	else
	{	
		retval = skybox.SampleLevel(linear_sampler, -V, 0);
	}

	//Do shading
	output[int2(dispatch_thread_id.xy)] = float4(retval, 1.f);
}

#define LIGHTS_REGISTER register(t3)

#include "fullscreen_quad.hlsl"
#include "pbr_util.hlsl"
#include "hdr_util.hlsl"
#include "lighting.hlsl"

Texture2D gbuffer_albedo_roughness : register(t0);
Texture2D gbuffer_normal_metallic : register(t1);
Texture2D gbuffer_depth : register(t2);
//Consider SRV for light buffer in register t3
TextureCube skybox : register(t4);
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
	float2 uv = float2(dispatch_thread_id.x / screen_size.x, 1.f - (dispatch_thread_id.y / screen_size.y));

	float2 screen_coord = int2(dispatch_thread_id.x, screen_size.y - dispatch_thread_id.y);

	const float depth_f = gbuffer_depth[screen_coord].r;

	// View position and camera position
	float3 pos = unpack_position(float2(uv.x, 1.f - uv.y), depth_f, inv_projection, inv_view);
	float3 camera_pos = float3(inv_view[0][3], inv_view[1][3], inv_view[2][3]);
	float3 V = normalize(camera_pos - pos);

	float3 retval;

	if(depth_f != 1.0f)
	{
		const float3 albedo = gbuffer_albedo_roughness[screen_coord].xyz;
		const float roughness = gbuffer_albedo_roughness[screen_coord].w;
		const float3 normal = gbuffer_normal_metallic[screen_coord].xyz;
		const float metallic = gbuffer_normal_metallic[screen_coord].w;
		const float3 sampled_irradiance = irradiance_map.SampleLevel(s0, normal, 0);

		float3 R = reflect(-V, normal);
		float3 F0 = float3(0.04f, 0.04f, 0.04f);
		F0 = lerp(F0, albedo, metallic);

		float3 res = float3(0.0f, 0.0f, 0.0f);
		
		uint light_count = lights[0].tid >> 2;	//Light count is stored in 30 upper-bits of first light

		for (uint i = 0; i < light_count; i++)
		{
			uint tid = lights[i].tid & 3;

			//Light direction (constant with directional, position dependent with other)
			float3 L = (lerp(lights[i].pos - pos, lights[i].dir, tid == light_type_directional));
			float light_dist = length(L);
			L /= light_dist;

			float3 H = normalize(V + L);

			//Spot intensity (only used with spot; but always calculated)
			float min_cos = cos(lights[i].ang);
			float max_cos = lerp(min_cos, 1, 0.5f);
			float cos_angle = dot(lights[i].dir, -L);
			float spot_intensity = lerp(smoothstep(min_cos, max_cos, cos_angle), 1, tid != light_type_spot);

			//Attenuation & spot intensity (only used with point or spot)
			float attenuation = lerp(1.0f - smoothstep(0, lights[i].rad, light_dist), 1, tid == light_type_directional);
			float3 radiance = (lights[i].col * spot_intensity) * attenuation;

			//cook-torrance BRDF
			float NDF = D_GGX(max(dot(normal, H), 0.0f), roughness);
			float G = G_SchlicksmithGGX(dot(normal, L), dot(normal, V), roughness);
			float3 F = F_Schlick(max(dot(H, V), 0.0f), F0);

			float3 nominator = NDF * G * F;
			float denominator = 4 * max(dot(normal, V), 0.0f) * max(dot(normal, L), 0.0f) + 0.001f;
			float3 specular = nominator / denominator;

			float3 kS = F;
			float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
			kD *= 1.0f - metallic;

			float NdotL = max(dot(normal, L), 0.0f);

			float3 lighting = (kD * albedo / PI + specular) * radiance * NdotL;

			res += lighting;
		}

		float3 kS = F_Schlick(max(dot(normal, V), 0.0f), F0);
		float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
		kD *= 1.0f - metallic;

		float3 irradiance = irradiance_map.SampleLevel(s0, normal, 0).xyz;
		float3 diffuse = irradiance * albedo;
		float3 ambient = (kD * diffuse) * 1.0f; //1.0f replaced with AO

		float3 color = ambient + res;

		retval = color;
	}
	else
	{	
		const float3 cpos = float3(inv_view[0][3], inv_view[1][3], inv_view[2][3]);
		const float3 cdir = normalize(cpos - pos);
		retval = skybox.SampleLevel(s0, cdir , 0);
	}
	
	float gamma = 1;
	float exposure = 1;
	retval = linearToneMapping(retval, exposure, gamma);

	//Do shading
	output[int2(dispatch_thread_id.xy)] = float4(retval, 1.f);
}

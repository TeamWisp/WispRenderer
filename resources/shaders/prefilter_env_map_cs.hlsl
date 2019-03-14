#include "pbr_util.hlsl"

TextureCube<float4> src_texture : register(t0);
RWTexture2D<float4> dst_texture : register(u0);
SamplerState s0 : register(s0);

cbuffer CB : register(b0)
{
	float2 texture_size;
	float2 skybox_res;
	float  roughness;
	uint   cubemap_face;
}

[numthreads(8, 8, 1)]
void main_cs(uint3 dt_id : SV_DispatchThreadID)
{
	float2 position = float2(dt_id.xy) / texture_size;
	position.y = 1.0f - position.y;
	position = (position - 0.5f) * 2.0f;

	float3 direction = float3(0.0f, 0.0f, 0.0f);
	float3 up = float3(0.0f, 0.0f, 0.0f);

	switch (cubemap_face)
	{
		case 0:  direction = float3(1.0f, position.y, -position.x);  up = float3(0.0f, 1.0f, 0.0f); break;	// +X
		case 1:  direction = float3(-1.0f, position.y, position.x);  up = float3(0.0f, 1.0f, 0.0f); break;	// -X
		case 2:  direction = float3(position.x, 1.0f, -position.y);  up = float3(0.0f, 0.0f, -1.0f); break;	// +Y
		case 3:  direction = float3(position.x, -1.0f, position.y);  up = float3(0.0f, 0.0f, 1.0f); break;	// -Y
		case 4:  direction = float3(position.x, position.y, 1.0f);   up = float3(0.0f, 1.0f, 0.0f); break;	// +Z
		case 5:  direction = float3(-position.x, position.y, -1.0f); up = float3(0.0f, 1.0f, 0.0f); break;	// -Z
	}

	float3 N = normalize(direction);
	float3 right = normalize(cross(up, N));
	up = cross(N, right);

	float3 R = N;
	float3 V = R;

	const uint SAMPLE_COUNT = 1024u;
	float total_weight = 0.0f;
	float3 prefiltered_color = float3(0.0f, 0.0f, 0.0f);

	for (uint i = 0u; i < SAMPLE_COUNT; i++)
	{
		float2 Xi = hammersley2d(i, SAMPLE_COUNT);
		float3 H = importanceSample_GGX(Xi, roughness, N);
		float3 L = normalize(2.0f * dot(V, H) * H - V);

		float NdotL = max(dot(N, L), 0.0f);
		if (NdotL > 0.0f)
		{
			float NdotH = max(dot(N, H), 0.0f);
			float HdotV = max(dot(H, V), 0.0f);

			float D = D_GGX(NdotH, roughness);

			float pdf = D * NdotH / (4.0f * HdotV) + 0.0001f;

			float sa_texel = 4.0f * PI / (6.0f * skybox_res.x * skybox_res.y);
			float sa_sample = 1.0f / (float(SAMPLE_COUNT) * pdf + 0.0001f);

			float mip_level = roughness == 0.0f ? 0.0f : 0.5f * log2(sa_sample / sa_texel);

			prefiltered_color += src_texture.SampleLevel(s0, L, mip_level).rgb * NdotL;
			total_weight += NdotL;
		}
	}

	prefiltered_color = prefiltered_color / total_weight;

	//Write the final color into the destination texture.
	dst_texture[dt_id.xy] = float4(prefiltered_color, 1.0f);
}
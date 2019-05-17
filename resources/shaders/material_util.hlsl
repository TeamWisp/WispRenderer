#define MATERIAL_HAS_ALBEDO_TEXTURE 1<<0
#define MATERIAL_HAS_NORMAL_TEXTURE 1<<1
#define MATERIAL_HAS_ROUGHNESS_TEXTURE 1<<2
#define MATERIAL_HAS_METALLIC_TEXTURE 1<<3
#define MATERIAL_HAS_EMISSIVE_TEXTURE 1<<4
#define MATERIAL_HAS_AO_TEXTURE 1<<5

struct MaterialData
{
	float3 color;
	float metallic;

	float roughness;
	float emissive_multiplier;
	float is_double_sided;
	float use_alpha_masking;

	float albedo_uv_scale;
	float normal_uv_scale;
	float roughness_uv_scale;
	float metallic_uv_scale;

	float emissive_uv_scale;
	float ao_uv_scale;
	float padding;
	uint flags;
};

struct OutputMaterialData
{
	float3 albedo;
	float alpha;
	float roughness;
	float3 normal;
	float metallic;
	float3 emissive;
	float ao;
};

OutputMaterialData InterpretMaterialData(MaterialData data,
	Texture2D material_albedo,
	Texture2D material_normal,
	Texture2D material_roughness,
	Texture2D material_metallic,
	Texture2D material_emissive,
	Texture2D material_ambient_occlusion,
	SamplerState s0,
	float2 uv
)
{
	OutputMaterialData output;

	float use_albedo_texture = float((data.flags & MATERIAL_HAS_ALBEDO_TEXTURE) != 0);
	float use_roughness_texture = float((data.flags & MATERIAL_HAS_ROUGHNESS_TEXTURE) != 0);
	float use_metallic_texture = float((data.flags & MATERIAL_HAS_METALLIC_TEXTURE) != 0);
	float use_normal_texture = float((data.flags & MATERIAL_HAS_NORMAL_TEXTURE) != 0);
	float use_emissive_texture = float((data.flags & MATERIAL_HAS_EMISSIVE_TEXTURE) != 0);
	float use_ao_texture = float((data.flags & MATERIAL_HAS_AO_TEXTURE) != 0);

	float4 albedo = lerp(float4(data.color, 1), material_albedo.Sample(s0, uv * data.albedo_uv_scale), use_albedo_texture);

	float roughness = lerp(data.roughness, max(0.05f, material_roughness.Sample(s0, uv * data.roughness_uv_scale).x), use_roughness_texture);

	float metallic = lerp(data.metallic, material_metallic.Sample(s0, uv * data.metallic_uv_scale).x, use_metallic_texture);

	float3 tex_normal = lerp(float3(0.0f, 0.0f, 1.0f), material_normal.Sample(s0, uv * data.normal_uv_scale).rgb * 2.0f - float3(1.0f, 1.0f, 1.0f), use_normal_texture);

	float3 emissive = lerp(float3(0.0f, 0.0f, 0.0f), material_emissive.Sample(s0, uv * data.emissive_uv_scale).xyz, use_emissive_texture);

	float ao = lerp(1.0f, material_ambient_occlusion.Sample(s0, uv * data.ao_uv_scale).x, use_ao_texture);

	output.albedo = pow(albedo.xyz, 2.2f);
	output.alpha = albedo.w;
	output.roughness = roughness;
	output.normal = tex_normal;
	output.metallic = metallic;
	output.emissive = pow(emissive,2.2f) * data.emissive_multiplier;
	output.ao = ao;

	return output;
}

OutputMaterialData InterpretMaterialDataRT(MaterialData data,
	Texture2D material_albedo,
	Texture2D material_normal,
	Texture2D material_roughness,
	Texture2D material_metallic,
	Texture2D material_emissive,
	Texture2D material_ambient_occlusion,
	float mip_level,
	SamplerState s0,
	float2 uv)
{
	OutputMaterialData output;

	float use_albedo_texture = float((data.flags & MATERIAL_HAS_ALBEDO_TEXTURE) != 0);
	float use_roughness_texture = float((data.flags & MATERIAL_HAS_ROUGHNESS_TEXTURE) != 0);
	float use_metallic_texture = float((data.flags & MATERIAL_HAS_METALLIC_TEXTURE) != 0);
	float use_normal_texture = float((data.flags & MATERIAL_HAS_NORMAL_TEXTURE) != 0);
	float use_emissive_texture = float((data.flags & MATERIAL_HAS_EMISSIVE_TEXTURE) != 0);
	float use_ao_texture = float((data.flags & MATERIAL_HAS_AO_TEXTURE) != 0);

	const float4 albedo = lerp(float4(data.color, 1),
		material_albedo.SampleLevel(s0, uv * (data.albedo_uv_scale), mip_level),
		use_albedo_texture);

	const float roughness = lerp(data.roughness,
		max(0.05, material_roughness.SampleLevel(s0, uv * data.roughness_uv_scale, mip_level).r),
		use_roughness_texture);

	const float metallic = lerp(data.metallic,
		material_metallic.SampleLevel(s0, uv * data.metallic_uv_scale, mip_level).r,
		use_metallic_texture);

	const float3 normal_t = lerp(float3(0.0, 0.0, 1.0),
		material_normal.SampleLevel(s0, uv * data.normal_uv_scale, mip_level).xyz * 2 - 1,
		use_normal_texture);

	float3 emissive = lerp(float3(0.0f, 0.0f, 0.0f), 
		material_emissive.SampleLevel(s0, uv * data.emissive_uv_scale, mip_level).xyz, use_emissive_texture);

	float ao = lerp(1.0f, 
		material_ambient_occlusion.SampleLevel(s0, uv * data.ao_uv_scale, mip_level).x, use_ao_texture);

	output.albedo = pow(albedo.xyz, 2.2f);
	output.alpha = albedo.w;
	output.roughness = roughness;
	output.normal = normal_t;
	output.metallic = metallic;
	output.emissive = pow(emissive, 2.2f) * data.emissive_multiplier;
	output.ao = ao;

	return output;
}

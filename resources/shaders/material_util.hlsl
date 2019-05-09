#define MATERIAL_HAS_ALBEDO_TEXTURE 1<<0
#define MATERIAL_HAS_NORMAL_TEXTURE 1<<1
#define MATERIAL_HAS_ROUGHNESS_TEXTURE 1<<2
#define MATERIAL_HAS_METALLIC_TEXTURE 1<<3
#define MATERIAL_HAS_AO_TEXTURE 1<<4

struct MaterialData
{
	float3 color;
	float metallic;

	float roughness;
	float is_double_sided;
	float use_alpha_masking;
	uint flags;
};

struct OutputMaterialData
{
	float3 albedo;
	float roughness;
	float3 normal;
	float metallic;
};

OutputMaterialData InterpretMaterialData(MaterialData data, 
	Texture2D material_albedo,
	Texture2D material_normal,
	Texture2D material_roughness,
	Texture2D material_metallic,
	SamplerState s0,
	float2 uv
	) 
{
	OutputMaterialData output;

	float use_albedo_texture = float((data.flags & MATERIAL_HAS_ALBEDO_TEXTURE) != 0);
	float use_roughness_texture = float((data.flags & MATERIAL_HAS_ROUGHNESS_TEXTURE) != 0);
	float use_metallic_texture = float((data.flags & MATERIAL_HAS_METALLIC_TEXTURE) != 0);
	float use_normal_texture = float((data.flags & MATERIAL_HAS_NORMAL_TEXTURE) != 0);

	float3 albedo = lerp(data.color, material_albedo.Sample(s0, uv).xyz, use_albedo_texture);

	//#define COMPRESSED_PBR
#ifdef COMPRESSED_PBR
	float roughness = lerp(data.roughness, material_metallic.Sample(s0, uv).y, use_roughness_texture);
	float metallic = lerp(data.metallic, material_metallic.Sample(s0, uv).z, use_metallic_texture);
#else
	float roughness = lerp(data.roughness, max(0.05f, material_roughness.Sample(s0, uv).x), use_roughness_texture);
	float metallic = lerp(data.metallic, material_metallic.Sample(s0, uv).x, use_metallic_texture);
#endif

	float3 tex_normal = lerp(float3(0.0, 0.0, 1.0), material_normal.Sample(s0, uv).rgb * 2.0 - float3(1.0, 1.0, 1.0), use_normal_texture);

	output.albedo = albedo;
	output.roughness = roughness;
	output.normal = tex_normal;
	output.metallic = metallic;

	return output;
}

OutputMaterialData InterpretMaterialDataRT(MaterialData data,
	Texture2D material_albedo,
	Texture2D material_normal,
	Texture2D material_roughness,
	Texture2D material_metallic,
	float mip_level,
	SamplerState s0,
	float2 uv)
{
	OutputMaterialData output;

	float use_albedo_texture = float((data.flags & MATERIAL_HAS_ALBEDO_TEXTURE) != 0);
	float use_roughness_texture = float((data.flags & MATERIAL_HAS_ROUGHNESS_TEXTURE) != 0);
	float use_metallic_texture = float((data.flags & MATERIAL_HAS_METALLIC_TEXTURE) != 0);
	float use_normal_texture = float((data.flags & MATERIAL_HAS_NORMAL_TEXTURE) != 0);

	//#define COMPRESSED_PBR
#ifdef COMPRESSED_PBR
	const float3 albedo = lerp(data.color,
		material_albedo.SampleLevel(s0, uv, mip_level).xyz,
		use_albedo_texture);

	const float roughness = lerp(data.roughness,
		max(0.05, material_metallic.SampleLevel(s0, uv, mip_level).y),
		use_roughness_texture);

	float metallic = lerp(data.metallic,
		material_metallic.SampleLevel(s0, uv, mip_level).z,
		use_metallic_texture);

	metallic = metallic * roughness;
	const float3 normal_t = lerp(,
		float3(0.0, 0.0, 1.0),
		material_normal.SampleLevel(s0, uv, mip_level).xyz * 2 - 1,
		use_normal_texture);
#else
	const float3 albedo = lerp(data.color,
		material_albedo.SampleLevel(s0, uv, mip_level).xyz,
		use_albedo_texture);

	const float roughness = lerp(data.roughness,
		max(0.05, material_roughness.SampleLevel(s0, uv, mip_level).r),
		use_roughness_texture);

	const float metallic = lerp(data.metallic,
		material_metallic.SampleLevel(s0, uv, mip_level).r,
		use_metallic_texture);

	const float3 normal_t = lerp(float3(0.0, 0.0, 1.0),
		material_normal.SampleLevel(s0, uv, mip_level).xyz * 2 - 1,
		use_normal_texture);
#endif

	output.albedo = albedo;
	output.roughness = roughness;
	output.normal = normal_t;
	output.metallic = metallic;
	return output;
}

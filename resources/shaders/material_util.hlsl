#define MATERIAL_HAS_ALBEDO_TEXTURE 1<<0;
#define MATERIAL_HAS_ALBEDO_CONSTANT 1<<1;
#define MATERIAL_USE_ALBEDO_CONSTANT 1<<2;
#define MATERIAL_HAS_NORMAL_TEXTURE 1<<3;
#define MATERIAL_HAS_ROUGHNESS_TEXTURE 1<<4;
#define MATERIAL_HAS_ROUGHNESS_CONSTANT 1<<5;
#define MATERIAL_USE_ROUGHNESS_CONSTANT 1<<6;
#define MATERIAL_HAS_METALLIC_TEXTURE 1<<7;
#define MATERIAL_HAS_METALLIC_CONSTANT 1<<8;
#define MATERIAL_USE_METALLIC_CONSTANT 1<<9;
#define MATERIAL_HAS_AO_TEXTURE 1<<10;
#define MATERIAL_HAS_ALPHA_MASK 1<<11;
#define MATERIAL_HAS_ALPHA_CONSTANT 1<<12;
#define MATERIAL_IS_DOUBLE_SIDED 1<<13;


struct MaterialData
{
	float4 albedo_alpha;
	float4 metallic_roughness;
	uint flags;
	uint3 padding;
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

	uint use_albedo_constant = data.flags & MATERIAL_USE_ALBEDO_CONSTANT;
	uint has_albedo_texture = data.flags & MATERIAL_HAS_ALBEDO_TEXTURE;
	uint use_roughness_constant = data.flags & MATERIAL_USE_ROUGHNESS_CONSTANT;
	uint has_roughness_texture = data.flags & MATERIAL_HAS_ROUGHNESS_TEXTURE;
	uint use_metallic_constant = data.flags & MATERIAL_USE_METALLIC_CONSTANT;
	uint has_metallic_texture = data.flags & MATERIAL_HAS_METALLIC_TEXTURE;
	uint use_normal_texture = data.flags & MATERIAL_HAS_NORMAL_TEXTURE;

	float4 albedo = lerp(material_albedo.Sample(s0, uv), float4(data.albedo_alpha.xyz, 1.0f), use_albedo_constant != 0 || has_albedo_texture == 0);

	//#define COMPRESSED_PBR
#ifdef COMPRESSED_PBR
	float4 roughness = lerp(material_metallic.SampleLevel(s0, input.uv, 0).y, data.metallic_roughness.w, use_roughness_constant != 0 || has_roughness_texture == 0);
	float4 metallic = lerp(material_metallic.SampleLevel(s0, input.uv, 0).z, length(data.metallic_roughness.xyz), use_metallic_constant != 0 || has_metallic_texture == 0);
#else
	float4 roughness = lerp(max(0.05f, material_roughness.Sample(s0, uv)), data.metallic_roughness.wwww, use_roughness_constant != 0 || has_roughness_texture == 0);
	float4 metallic = lerp(material_metallic.Sample(s0, uv), data.metallic_roughness.xyzx, use_metallic_constant != 0 || has_metallic_texture == 0);
#endif
	float3 tex_normal = lerp(material_normal.Sample(s0, uv).rgb * 2.0 - float3(1.0, 1.0, 1.0), float3(0.0, 0.0, 1.0), use_normal_texture == 0);

	output.albedo = albedo.xyz;
	output.roughness = roughness.x;
	output.normal = tex_normal;
	output.metallic = metallic.x;

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

	uint use_albedo_constant = data.flags & MATERIAL_USE_ALBEDO_CONSTANT;
	uint has_albedo_texture = data.flags & MATERIAL_HAS_ALBEDO_TEXTURE;
	uint use_roughness_constant = data.flags & MATERIAL_USE_ROUGHNESS_CONSTANT;
	uint has_roughness_texture = data.flags & MATERIAL_HAS_ROUGHNESS_TEXTURE;
	uint use_metallic_constant = data.flags & MATERIAL_USE_METALLIC_CONSTANT;
	uint has_metallic_texture = data.flags & MATERIAL_HAS_METALLIC_TEXTURE;
	uint use_normal_texture = data.flags & MATERIAL_HAS_NORMAL_TEXTURE;

	//#define COMPRESSED_PBR
#ifdef COMPRESSED_PBR
	const float3 albedo = lerp(material_albedo.SampleLevel(s0, uv, mip_level).xyz,
		data.albedo_alpha.xyz,
		use_albedo_constant != 0 || has_albedo_texture == 0);
	const float roughness = lerp(max(0.05, material_metallic.SampleLevel(s0, uv, mip_level).y),
		data.metallic_roughness.w,
		use_roughness_constant != 0 || has_roughness_texture == 0);
	float metal = lerp(material_metallic.SampleLevel(s0, uv, mip_level).z,
		length(data.metallic_roughness.xyz),
		use_metallic_constant != 0 || has_metallic_texture == 0);
	metal = metal * roughness;
	const float3 normal_t = lerp((material_normal.SampleLevel(s0, uv, mip_level).xyz) * 2.0 - float3(1.0, 1.0, 1.0),
		float3(0.0, 0.0, 1.0),
		use_normal_texture == 0);
#else
	const float3 albedo = lerp(material_albedo.SampleLevel(s0, uv, mip_level).xyz,
		data.albedo_alpha.xyz,
		use_albedo_constant != 0 || has_albedo_texture == 0);
	const float roughness = lerp(max(0.05, material_roughness.SampleLevel(s0, uv, mip_level).r),
		data.metallic_roughness.w,
		use_roughness_constant != 0 || has_roughness_texture == 0);
	const float metal = lerp(material_metallic.SampleLevel(s0, uv, mip_level).r,
		length(data.metallic_roughness.xyz),
		use_metallic_constant != 0 || has_metallic_texture == 0);
	const float3 normal_t = lerp((material_normal.SampleLevel(s0, uv, mip_level).xyz * 2.0) - float3(1.0, 1.0, 1.0),
		float3(0.0, 0.0, 1.0),
		use_normal_texture == 0);
#endif

	output.albedo = albedo;
	output.roughness = roughness;
	output.normal = normal_t;
	output.metallic = metal;
	return output;
}
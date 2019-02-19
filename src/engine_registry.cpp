#pragma once

#include "engine_registry.hpp"

#include "pipeline_registry.hpp"
#include "root_signature_registry.hpp"
#include "shader_registry.hpp"
#include "rt_pipeline_registry.hpp"
#include "d3d12/d3d12_structs.hpp"

// Register something into a registry.
#define REGISTER(type, registry) decltype(type) type = registry::Get().Register
// Decscriptor Range Array
#define DESC_RANGE_ARRAY(name, ...) std::vector<CD3DX12_DESCRIPTOR_RANGE> name { __VA_ARGS__ };
// Descriptor Range
#define DESC_RANGE(...) [] { return GetRange(__VA_ARGS__); }()
// Descriptor Range Hardcoded
#define DESC_RANGE_H(...) [] { CD3DX12_DESCRIPTOR_RANGE r; r.Init(__VA_ARGS__); return r; }()
// Root parameter
#define ROOT_PARAM(func) [] { return func; }()
// Root paramter hard coded
#define ROOT_PARAM_H(func) [] { CD3DX12_ROOT_PARAMETER d; d.func; return d; }()
// Root Parameter for descriptor tables
#define ROOT_PARAM_DESC_TABLE(arr, visibility) [] { CD3DX12_ROOT_PARAMETER d; d.InitAsDescriptorTable(arr.size(), arr.data(), visibility); return d; }()

namespace wr
{
	using namespace rs_layout;

	//Basic Deferred Pass Root Signature
	DESC_RANGE_ARRAY(ranges_basic,
		DESC_RANGE(params::basic, Type::SRV_RANGE, params::BasicE::ALBEDO),
		DESC_RANGE(params::basic, Type::SRV_RANGE, params::BasicE::NORMAL),
		DESC_RANGE(params::basic, Type::SRV_RANGE, params::BasicE::ROUGHNESS),
		DESC_RANGE(params::basic, Type::SRV_RANGE, params::BasicE::METALLIC),
	);

	REGISTER(root_signatures::basic, RootSignatureRegistry)({
		{
			ROOT_PARAM(GetCBV(params::basic, params::BasicE::CAMERA_PROPERTIES, D3D12_SHADER_VISIBILITY_VERTEX)),
			ROOT_PARAM(GetCBV(params::basic, params::BasicE::OBJECT_PROPERTIES, D3D12_SHADER_VISIBILITY_VERTEX)),
			ROOT_PARAM_DESC_TABLE(ranges_basic, D3D12_SHADER_VISIBILITY_PIXEL),
			ROOT_PARAM(GetCBV(params::basic,params::BasicE::MATERIAL_PROPERTIES,D3D12_SHADER_VISIBILITY_PIXEL)),
		},
		{
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_WRAP }
		}
	});

	//Deferred Composition Root Signature
	DESC_RANGE_ARRAY(srv_ranges,
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::GBUFFER_ALBEDO_ROUGHNESS),
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::GBUFFER_NORMAL_METALLIC),
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::GBUFFER_DEPTH),
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::LIGHT_BUFFER),
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::SKY_BOX),
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::IRRADIANCE_MAP),
		DESC_RANGE(params::deferred_composition, Type::UAV_RANGE, params::DeferredCompositionE::OUTPUT),
	);

	REGISTER(root_signatures::deferred_composition, RootSignatureRegistry)({
		{
			ROOT_PARAM(GetCBV(params::deferred_composition, params::DeferredCompositionE::CAMERA_PROPERTIES)),
			ROOT_PARAM_DESC_TABLE(srv_ranges, D3D12_SHADER_VISIBILITY_ALL),
		},
		{
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_CLAMP }
		}
	});


	//MipMapping Root Signature
	DESC_RANGE_ARRAY(mip_in_out_ranges,
		DESC_RANGE(params::mip_mapping, Type::SRV_RANGE, params::MipMappingE::SOURCE),
		DESC_RANGE(params::mip_mapping, Type::UAV_RANGE, params::MipMappingE::DEST),
	);
	REGISTER(root_signatures::mip_mapping, RootSignatureRegistry)({
		{
			ROOT_PARAM(GetConstants(params::mip_mapping, params::MipMappingE::TEXEL_SIZE)),
			ROOT_PARAM_DESC_TABLE(mip_in_out_ranges, D3D12_SHADER_VISIBILITY_ALL)
		},
		{
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP }
		}
	});


	//Cubemap conversion root signature
	DESC_RANGE_ARRAY(cubemap_tasks_ranges,
		DESC_RANGE(params::cubemap_conversion, Type::SRV_RANGE, params::CubemapConversionE::EQUIRECTANGULAR_TEXTURE),
	);
	REGISTER(root_signatures::cubemap_conversion, RootSignatureRegistry)({
		{
			ROOT_PARAM(GetConstants(params::cubemap_conversion, params::CubemapConversionE::IDX)),
			ROOT_PARAM(GetCBV(params::cubemap_conversion, params::CubemapConversionE::CAMERA_PROPERTIES)),
			ROOT_PARAM_DESC_TABLE(cubemap_tasks_ranges, D3D12_SHADER_VISIBILITY_PIXEL),
		},
		{
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP }
		}
	});
	
	//Cubemap convolution root signature
	DESC_RANGE_ARRAY(cubemap_convolution_ranges,
		DESC_RANGE(params::cubemap_conversion, Type::SRV_RANGE, params::CubemapConvolutionE::ENVIRONMENT_CUBEMAP),
	);
	REGISTER(root_signatures::cubemap_convolution, RootSignatureRegistry)({
		{
			ROOT_PARAM(GetConstants(params::cubemap_convolution, params::CubemapConvolutionE::IDX)),
			ROOT_PARAM(GetCBV(params::cubemap_convolution, params::CubemapConvolutionE::CAMERA_PROPERTIES)),
			ROOT_PARAM_DESC_TABLE(cubemap_tasks_ranges, D3D12_SHADER_VISIBILITY_PIXEL),
		},
		{
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP }
		}
	});


	REGISTER(shaders::basic_vs, ShaderRegistry)({
		"resources/shaders/basic.hlsl",
		"main_vs",
		ShaderType::VERTEX_SHADER
	});

	REGISTER(shaders::basic_ps, ShaderRegistry)({
		"resources/shaders/basic.hlsl",
		"main_ps",
		ShaderType::PIXEL_SHADER
	});

	REGISTER(shaders::fullscreen_quad_vs, ShaderRegistry)({
		"resources/shaders/fullscreen_quad.hlsl",
		"main_vs",
		ShaderType::VERTEX_SHADER
	});

	REGISTER(shaders::deferred_composition_cs, ShaderRegistry)({
		"resources/shaders/deferred_composition.hlsl",
		"main_cs",
		ShaderType::DIRECT_COMPUTE_SHADER
	});

	REGISTER(shaders::mip_mapping_cs, ShaderRegistry)({
		"resources/shaders/generate_mips_cs.hlsl",
		"main",
		ShaderType::DIRECT_COMPUTE_SHADER
	});

	REGISTER(shaders::equirect_to_cubemap_vs, ShaderRegistry)({
		"resources/shaders/equirect_to_cubemap_conversion.hlsl",
		"main_vs",
		ShaderType::VERTEX_SHADER
	});

	REGISTER(shaders::equirect_to_cubemap_ps, ShaderRegistry)({
		"resources/shaders/equirect_to_cubemap_conversion.hlsl",
		"main_ps",
		ShaderType::PIXEL_SHADER
	});

	REGISTER(shaders::cubemap_convolution_ps, ShaderRegistry)({
		"resources/shaders/cubemap_convolution.hlsl",
		"main_ps",
		ShaderType::PIXEL_SHADER
	});


	REGISTER(pipelines::basic_deferred, PipelineRegistry)<VertexColor>({
		shaders::basic_vs,
		shaders::basic_ps,
		std::nullopt,
		root_signatures::basic,
		Format::D32_FLOAT,
		{ Format::R32G32B32A32_FLOAT, Format::R32G32B32A32_FLOAT },
		3,
		PipelineType::GRAPHICS_PIPELINE,
		CullMode::CULL_BACK,
		true,
		false,
		TopologyType::TRIANGLE
	});

	REGISTER(pipelines::deferred_composition, PipelineRegistry)<Vertex2D>({
		std::nullopt,
		std::nullopt,
		shaders::deferred_composition_cs,
		root_signatures::deferred_composition,
		Format::UNKNOWN,
		{ Format::R8G8B8A8_UNORM },
		1,
		PipelineType::COMPUTE_PIPELINE,
		CullMode::CULL_BACK,
		false,
		true,
		TopologyType::TRIANGLE
	});

	REGISTER(pipelines::mip_mapping, PipelineRegistry)<VertexColor>(
	{
			std::nullopt,
			std::nullopt,
			shaders::mip_mapping_cs,
			root_signatures::mip_mapping,
			Format::UNKNOWN,
			{ }, //This compute shader doesn't use any render target
			0,
			PipelineType::COMPUTE_PIPELINE,
			CullMode::CULL_BACK,
			false,
			true,
			TopologyType::TRIANGLE
	});

	REGISTER(pipelines::equirect_to_cubemap, PipelineRegistry)<Vertex>(
	{
		shaders::equirect_to_cubemap_vs,
		shaders::equirect_to_cubemap_ps,
		std::nullopt,
		root_signatures::cubemap_conversion,
		Format::UNKNOWN,
		{ Format::R32G32B32A32_FLOAT },
		1,
		PipelineType::GRAPHICS_PIPELINE,
		CullMode::CULL_NONE,
		false,
		false,
		TopologyType::TRIANGLE
	});

	REGISTER(pipelines::cubemap_convolution, PipelineRegistry)<Vertex>(
	{
		shaders::equirect_to_cubemap_vs,
		shaders::cubemap_convolution_ps,
		std::nullopt,
		root_signatures::cubemap_convolution,
		Format::UNKNOWN,
		{ Format::R32G32B32A32_FLOAT },
		1,
		PipelineType::GRAPHICS_PIPELINE,
		CullMode::CULL_NONE,
		false,
		false,
		TopologyType::TRIANGLE
	});


	/* ### Raytracing ### */
	REGISTER(shaders::post_processing, ShaderRegistry)({
		"resources/shaders/post_processing.hlsl",
		"main",
		ShaderType::DIRECT_COMPUTE_SHADER
	});

	REGISTER(shaders::rt_lib, ShaderRegistry)({
		"resources/shaders/raytracing.hlsl",
		"RaygenEntry",
		ShaderType::LIBRARY_SHADER
	});

	DESC_RANGE_ARRAY(accum_r,
		DESC_RANGE(params::post_processing, Type::SRV_RANGE, params::PostProcessingE::SOURCE),
		DESC_RANGE(params::post_processing, Type::UAV_RANGE, params::PostProcessingE::DEST),
	);

	REGISTER(root_signatures::post_processing, RootSignatureRegistry)({
		{
			ROOT_PARAM_DESC_TABLE(accum_r, D3D12_SHADER_VISIBILITY_ALL),
			ROOT_PARAM(GetConstants(params::post_processing, params::PostProcessingE::HDR_SUPPORT)),
		},
		{
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_BORDER }
		}
	});

	REGISTER(pipelines::post_processing, PipelineRegistry)<Vertex2D>(
	{
		std::nullopt,
		std::nullopt,
		shaders::post_processing,
		root_signatures::post_processing,
		Format::UNKNOWN,
		{ d3d12::settings::back_buffer_format }, //This compute shader doesn't use any render target
		1,
		PipelineType::COMPUTE_PIPELINE,
		CullMode::CULL_NONE,
		false,
		true,
		TopologyType::TRIANGLE
	});

	DESC_RANGE_ARRAY(r,
		DESC_RANGE(params::full_raytracing, Type::UAV_RANGE, params::FullRaytracingE::OUTPUT),
		DESC_RANGE(params::full_raytracing, Type::SRV_RANGE, params::FullRaytracingE::INDICES),
		DESC_RANGE(params::full_raytracing, Type::SRV_RANGE, params::FullRaytracingE::LIGHTS),
		DESC_RANGE(params::full_raytracing, Type::SRV_RANGE, params::FullRaytracingE::MATERIALS),
		DESC_RANGE(params::full_raytracing, Type::SRV_RANGE, params::FullRaytracingE::OFFSETS),
		DESC_RANGE(params::full_raytracing, Type::SRV_RANGE, params::FullRaytracingE::SKYBOX),
		DESC_RANGE(params::full_raytracing, Type::SRV_RANGE, params::FullRaytracingE::TEXTURES),
		DESC_RANGE_H(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, d3d12::settings::fallback_ptrs_offset),
	);

	REGISTER(root_signatures::rt_test_global, RootSignatureRegistry)({
		{
			ROOT_PARAM_DESC_TABLE(r, D3D12_SHADER_VISIBILITY_ALL),
			ROOT_PARAM(GetSRV(params::full_raytracing, params::FullRaytracingE::ACCELERATION_STRUCTURE)),
			ROOT_PARAM(GetCBV(params::full_raytracing, params::FullRaytracingE::CAMERA_PROPERTIES)),
			ROOT_PARAM(GetSRV(params::full_raytracing, params::FullRaytracingE::VERTICES)),
		},
		{
			{ TextureFilter::FILTER_ANISOTROPIC, TextureAddressMode::TAM_WRAP }
		},
		true // rtx
	});

	std::pair<CD3DX12_STATE_OBJECT_DESC, StateObjectDescription::Library> so_desc = []()
	{
		CD3DX12_STATE_OBJECT_DESC desc = { D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

		StateObjectDescription::Library lib;
		lib.shader_handle = shaders::rt_lib;
		lib.exports.push_back(L"RaygenEntry");
		lib.exports.push_back(L"ClosestHitEntry");
		lib.exports.push_back(L"MissEntry");
		lib.exports.push_back(L"ShadowClosestHitEntry");
		lib.exports.push_back(L"ShadowMissEntry");
		lib.m_hit_groups.push_back({ L"MyHitGroup", L"ClosestHitEntry" });
		lib.m_hit_groups.push_back({ L"ShadowHitGroup", L"ShadowClosestHitEntry" });

		return std::make_pair(desc, lib);
	}();
	
	REGISTER(state_objects::state_object, RTPipelineRegistry)(
	{
		so_desc.first,     // Description
		so_desc.second,    // Library
		(sizeof(float)* 7) + sizeof(unsigned int), // Max payload size
		(sizeof(float)* 4), // Max attributes size
		3,				   // Max recursion depth
		root_signatures::rt_test_global,      // Global root signature
		std::nullopt,      // Local Root Signatures
	});

	/* ### Hybrid Raytracing ### */
	REGISTER(shaders::rt_hybrid_lib, ShaderRegistry)({
		"resources/shaders/rt_hybrid.hlsl",
		"RaygenEntry",
		ShaderType::LIBRARY_SHADER
	});

	DESC_RANGE_ARRAY(rt_hybrid_ranges,
		DESC_RANGE(params::rt_hybrid, Type::UAV_RANGE, params::RTHybridE::OUTPUT),
		DESC_RANGE(params::rt_hybrid, Type::SRV_RANGE, params::RTHybridE::INDICES),
		DESC_RANGE(params::rt_hybrid, Type::SRV_RANGE, params::RTHybridE::LIGHTS),
		DESC_RANGE(params::rt_hybrid, Type::SRV_RANGE, params::RTHybridE::MATERIALS),
		DESC_RANGE(params::rt_hybrid, Type::SRV_RANGE, params::RTHybridE::OFFSETS),
		DESC_RANGE(params::rt_hybrid, Type::SRV_RANGE, params::RTHybridE::SKYBOX),
		DESC_RANGE(params::rt_hybrid, Type::SRV_RANGE, params::RTHybridE::TEXTURES),
		DESC_RANGE(params::rt_hybrid, Type::SRV_RANGE, params::RTHybridE::GBUFFERS),
		DESC_RANGE_H(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, d3d12::settings::fallback_ptrs_offset),
	);

	REGISTER(root_signatures::rt_hybrid_global, RootSignatureRegistry)({
		{
			ROOT_PARAM_DESC_TABLE(rt_hybrid_ranges, D3D12_SHADER_VISIBILITY_ALL),
			ROOT_PARAM(GetSRV(params::rt_hybrid, params::RTHybridE::ACCELERATION_STRUCTURE)),
			ROOT_PARAM(GetCBV(params::rt_hybrid, params::RTHybridE::CAMERA_PROPERTIES)),
			ROOT_PARAM(GetSRV(params::rt_hybrid, params::RTHybridE::VERTICES)),
		},
		{
			{ TextureFilter::FILTER_ANISOTROPIC, TextureAddressMode::TAM_WRAP }
		},
		true // rtx
	});

	std::pair<CD3DX12_STATE_OBJECT_DESC, StateObjectDescription::Library> rt_hybrid_so_desc = []()
	{
		CD3DX12_STATE_OBJECT_DESC desc = { D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

		StateObjectDescription::Library lib;
		lib.shader_handle = shaders::rt_hybrid_lib;
		lib.exports.push_back(L"RaygenEntry");
		lib.exports.push_back(L"ShadowHit");
		lib.exports.push_back(L"ShadowMiss");
		lib.exports.push_back(L"ReflectionHit");
		lib.exports.push_back(L"ReflectionMiss");
		lib.m_hit_groups.push_back({L"ShadowHitGroup", L"ShadowHit"});
		lib.m_hit_groups.push_back({L"ReflectionHitGroup", L"ReflectionHit"});

		return std::make_pair(desc, lib);
	}();

	REGISTER(state_objects::rt_hybrid_state_object, RTPipelineRegistry)(
	{
		rt_hybrid_so_desc.first,     // Description
		rt_hybrid_so_desc.second,    // Library
		(sizeof(float) * 6), // Max payload size
		(sizeof(float) * 2), // Max attributes size
		2,				   // Max recursion depth
		root_signatures::rt_hybrid_global,      // Global root signature
		std::nullopt,      // Local Root Signatures
	});

} /* wr */

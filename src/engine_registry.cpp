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
		RootSignatureDescription::Parameters({
			ROOT_PARAM(GetCBV(params::basic, params::BasicE::CAMERA_PROPERTIES, D3D12_SHADER_VISIBILITY_VERTEX)),
			ROOT_PARAM(GetCBV(params::basic, params::BasicE::OBJECT_PROPERTIES, D3D12_SHADER_VISIBILITY_VERTEX)),
			ROOT_PARAM_DESC_TABLE(ranges_basic, D3D12_SHADER_VISIBILITY_PIXEL),
			ROOT_PARAM(GetCBV(params::basic,params::BasicE::MATERIAL_PROPERTIES,D3D12_SHADER_VISIBILITY_PIXEL)),
		}),
		RootSignatureDescription::Samplers({
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_WRAP }
		})
	});

	//Deferred Composition Root Signature
	DESC_RANGE_ARRAY(srv_ranges,
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::GBUFFER_ALBEDO_ROUGHNESS),
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::GBUFFER_NORMAL_METALLIC),
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::GBUFFER_DEPTH),
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::LIGHT_BUFFER),
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::SKY_BOX),
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::IRRADIANCE_MAP),
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::BUFFER_REFLECTION_SHADOW),
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::BUFFER_SCREEN_SPACE_IRRADIANCE),
		DESC_RANGE(params::deferred_composition, Type::UAV_RANGE, params::DeferredCompositionE::OUTPUT),
	);

	REGISTER(root_signatures::deferred_composition, RootSignatureRegistry)({
		RootSignatureDescription::Parameters({
			ROOT_PARAM(GetCBV(params::deferred_composition, params::DeferredCompositionE::CAMERA_PROPERTIES)),
			ROOT_PARAM_DESC_TABLE(srv_ranges, D3D12_SHADER_VISIBILITY_ALL),
		}),
		RootSignatureDescription::Samplers({
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_CLAMP }
		})
	});

	//MipMapping Root Signature
	DESC_RANGE_ARRAY(mip_in_out_ranges,
		DESC_RANGE(params::mip_mapping, Type::SRV_RANGE, params::MipMappingE::SOURCE),
		DESC_RANGE(params::mip_mapping, Type::UAV_RANGE, params::MipMappingE::DEST),
	);
	REGISTER(root_signatures::mip_mapping, RootSignatureRegistry)({
		RootSignatureDescription::Parameters({
			ROOT_PARAM(GetConstants(params::mip_mapping, params::MipMappingE::CBUFFER)),
			ROOT_PARAM_DESC_TABLE(mip_in_out_ranges, D3D12_SHADER_VISIBILITY_ALL)
		}),
		RootSignatureDescription::Samplers({
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP }
		})
	});


	//Cubemap conversion root signature
	DESC_RANGE_ARRAY(cubemap_tasks_ranges,
		DESC_RANGE(params::cubemap_conversion, Type::SRV_RANGE, params::CubemapConversionE::EQUIRECTANGULAR_TEXTURE),
	);
	REGISTER(root_signatures::cubemap_conversion, RootSignatureRegistry)({
		RootSignatureDescription::Parameters({
			ROOT_PARAM(GetConstants(params::cubemap_conversion, params::CubemapConversionE::IDX)),
			ROOT_PARAM(GetCBV(params::cubemap_conversion, params::CubemapConversionE::CAMERA_PROPERTIES)),
			ROOT_PARAM_DESC_TABLE(cubemap_tasks_ranges, D3D12_SHADER_VISIBILITY_PIXEL),
		}),
		RootSignatureDescription::Samplers({
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP }
		})
	});
	
	//Cubemap convolution root signature
	DESC_RANGE_ARRAY(cubemap_convolution_ranges,
		DESC_RANGE(params::cubemap_conversion, Type::SRV_RANGE, params::CubemapConvolutionE::ENVIRONMENT_CUBEMAP),
	);
	REGISTER(root_signatures::cubemap_convolution, RootSignatureRegistry)({
		RootSignatureDescription::Parameters({
			ROOT_PARAM(GetConstants(params::cubemap_convolution, params::CubemapConvolutionE::IDX)),
			ROOT_PARAM(GetCBV(params::cubemap_convolution, params::CubemapConvolutionE::CAMERA_PROPERTIES)),
			ROOT_PARAM_DESC_TABLE(cubemap_tasks_ranges, D3D12_SHADER_VISIBILITY_PIXEL),
		}),
		RootSignatureDescription::Samplers({
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP }
		})
	});


	REGISTER(shaders::basic_vs, ShaderRegistry)({
		ShaderDescription::Path("resources/shaders/basic.hlsl"),
		ShaderDescription::Entry("main_vs"),
		ShaderDescription::Type(ShaderType::VERTEX_SHADER)
	});

	REGISTER(shaders::basic_ps, ShaderRegistry)({
		ShaderDescription::Path("resources/shaders/basic.hlsl"),
		ShaderDescription::Entry("main_ps"),
		ShaderDescription::Type(ShaderType::PIXEL_SHADER)
	});

	REGISTER(shaders::fullscreen_quad_vs, ShaderRegistry)({
		ShaderDescription::Path("resources/shaders/fullscreen_quad.hlsl"),
		ShaderDescription::Entry("main_vs"),
		ShaderDescription::Type(ShaderType::VERTEX_SHADER)
	});

	REGISTER(shaders::deferred_composition_cs, ShaderRegistry)({
		ShaderDescription::Path("resources/shaders/deferred_composition.hlsl"),
		ShaderDescription::Entry("main_cs"),
		ShaderDescription::Type(ShaderType::DIRECT_COMPUTE_SHADER)
	});

	REGISTER(shaders::mip_mapping_cs, ShaderRegistry)({
		ShaderDescription::Path("resources/shaders/generate_mips_cs.hlsl"),
		ShaderDescription::Entry("main"),
		ShaderDescription::Type(ShaderType::DIRECT_COMPUTE_SHADER)
	});

	REGISTER(shaders::equirect_to_cubemap_vs, ShaderRegistry)({
		ShaderDescription::Path("resources/shaders/equirect_to_cubemap_conversion.hlsl"),
		ShaderDescription::Entry("main_vs"),
		ShaderDescription::Type(ShaderType::VERTEX_SHADER)
	});

	REGISTER(shaders::equirect_to_cubemap_ps, ShaderRegistry)({
		ShaderDescription::Path("resources/shaders/equirect_to_cubemap_conversion.hlsl"),
		ShaderDescription::Entry("main_ps"),
		ShaderDescription::Type(ShaderType::PIXEL_SHADER)
	});

	REGISTER(shaders::cubemap_convolution_ps, ShaderRegistry)({
		ShaderDescription::Path("resources/shaders/cubemap_convolution.hlsl"),
		ShaderDescription::Entry("main_ps"),
		ShaderDescription::Type(ShaderType::PIXEL_SHADER)
	});

	REGISTER(pipelines::basic_deferred, PipelineRegistry)<VertexColor>({
		PipelineDescription::VertexShader(shaders::basic_vs),
		PipelineDescription::PixelShader(shaders::basic_ps),
		PipelineDescription::ComputeShader(std::nullopt),
		PipelineDescription::RootSignature(root_signatures::basic),
		PipelineDescription::DSVFormat(Format::D32_FLOAT),
		PipelineDescription::RTVFormats({ Format::R32G32B32A32_FLOAT, Format::R32G32B32A32_FLOAT }),
		PipelineDescription::NumRTVFormats(3),
		PipelineDescription::Type(PipelineType::GRAPHICS_PIPELINE),
		PipelineDescription::CullMode(CullMode::CULL_NONE),
		PipelineDescription::Depth(true),
		PipelineDescription::CounterClockwise(false),
		PipelineDescription::TopologyType(TopologyType::TRIANGLE)
	});

	REGISTER(pipelines::deferred_composition, PipelineRegistry)<Vertex2D>({
		PipelineDescription::VertexShader(std::nullopt),
		PipelineDescription::PixelShader(std::nullopt),
		PipelineDescription::ComputeShader(shaders::deferred_composition_cs),
		PipelineDescription::RootSignature(root_signatures::deferred_composition),
		PipelineDescription::DSVFormat(Format::UNKNOWN),
		PipelineDescription::RTVFormats({ Format::R8G8B8A8_UNORM }),
		PipelineDescription::NumRTVFormats(1),
		PipelineDescription::Type(PipelineType::COMPUTE_PIPELINE),
		PipelineDescription::CullMode(CullMode::CULL_BACK),
		PipelineDescription::Depth(false),
		PipelineDescription::CounterClockwise(true),
		PipelineDescription::TopologyType(TopologyType::TRIANGLE)
	});

	REGISTER(pipelines::mip_mapping, PipelineRegistry)<VertexColor>({
		PipelineDescription::VertexShader(std::nullopt),
		PipelineDescription::PixelShader(std::nullopt),
		PipelineDescription::ComputeShader(shaders::mip_mapping_cs),
		PipelineDescription::RootSignature(root_signatures::mip_mapping),
		PipelineDescription::DSVFormat(Format::UNKNOWN),
		PipelineDescription::RTVFormats({ }),
		PipelineDescription::NumRTVFormats(0),
		PipelineDescription::Type(PipelineType::COMPUTE_PIPELINE),
		PipelineDescription::CullMode(CullMode::CULL_BACK),
		PipelineDescription::Depth(false),
		PipelineDescription::CounterClockwise(true),
		PipelineDescription::TopologyType(TopologyType::TRIANGLE)
	});

	REGISTER(pipelines::equirect_to_cubemap, PipelineRegistry)<Vertex>(
	{
		PipelineDescription::VertexShader(shaders::equirect_to_cubemap_vs),
		PipelineDescription::PixelShader(shaders::equirect_to_cubemap_ps),
		PipelineDescription::ComputeShader(std::nullopt),
		PipelineDescription::RootSignature(root_signatures::cubemap_conversion),
		PipelineDescription::DSVFormat(Format::UNKNOWN),
		PipelineDescription::RTVFormats({ Format::R32G32B32A32_FLOAT }),
		PipelineDescription::NumRTVFormats(1),
		PipelineDescription::Type(PipelineType::GRAPHICS_PIPELINE),
		PipelineDescription::CullMode(CullMode::CULL_NONE),
		PipelineDescription::Depth(false),
		PipelineDescription::CounterClockwise(false),
		PipelineDescription::TopologyType(TopologyType::TRIANGLE)
	});

	REGISTER(pipelines::cubemap_convolution, PipelineRegistry)<Vertex>(
	{
		PipelineDescription::VertexShader(shaders::equirect_to_cubemap_vs),
		PipelineDescription::PixelShader(shaders::cubemap_convolution_ps),
		PipelineDescription::ComputeShader(std::nullopt),
		PipelineDescription::RootSignature(root_signatures::cubemap_convolution),
		PipelineDescription::DSVFormat(Format::UNKNOWN),
		PipelineDescription::RTVFormats({ Format::R32G32B32A32_FLOAT }),
		PipelineDescription::NumRTVFormats(1),
		PipelineDescription::Type(PipelineType::GRAPHICS_PIPELINE),
		PipelineDescription::CullMode(CullMode::CULL_NONE),
		PipelineDescription::Depth(false),
		PipelineDescription::CounterClockwise(false),
		PipelineDescription::TopologyType(TopologyType::TRIANGLE)
	});


	/* ### Raytracing ### */
	REGISTER(shaders::post_processing, ShaderRegistry)({
		ShaderDescription::Path("resources/shaders/post_processing.hlsl"),
		ShaderDescription::Entry("main"),
		ShaderDescription::Type(ShaderType::DIRECT_COMPUTE_SHADER)
	});

	REGISTER(shaders::accumulation, ShaderRegistry)({
		ShaderDescription::Path("resources/shaders/accumulation.hlsl"),
		ShaderDescription::Entry("main"),
		ShaderDescription::Type(ShaderType::DIRECT_COMPUTE_SHADER)
	});

	REGISTER(shaders::rt_lib, ShaderRegistry)({
		ShaderDescription::Path("resources/shaders/raytracing.hlsl"),
		ShaderDescription::Entry("RaygenEntry"),
		ShaderDescription::Type(ShaderType::LIBRARY_SHADER)
	});

	DESC_RANGE_ARRAY(post_r,
		DESC_RANGE(params::post_processing, Type::SRV_RANGE, params::PostProcessingE::SOURCE),
		DESC_RANGE(params::post_processing, Type::UAV_RANGE, params::PostProcessingE::DEST),
	);

	REGISTER(root_signatures::post_processing, RootSignatureRegistry)({
		RootSignatureDescription::Parameters({
			ROOT_PARAM_DESC_TABLE(post_r, D3D12_SHADER_VISIBILITY_ALL),
			ROOT_PARAM(GetConstants(params::post_processing, params::PostProcessingE::HDR_SUPPORT)),
		}),
		RootSignatureDescription::Samplers({
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_BORDER }
		})
	});

	DESC_RANGE_ARRAY(accum_r,
		DESC_RANGE(params::accumulation, Type::SRV_RANGE, params::PostProcessingE::SOURCE),
		DESC_RANGE(params::accumulation, Type::UAV_RANGE, params::PostProcessingE::DEST),
	);

	REGISTER(root_signatures::accumulation, RootSignatureRegistry)({
		RootSignatureDescription::Parameters({
			ROOT_PARAM_DESC_TABLE(accum_r, D3D12_SHADER_VISIBILITY_ALL),
			ROOT_PARAM(GetConstants(params::accumulation, params::AccumulationE::FRAME_IDX)),
		}),
		RootSignatureDescription::Samplers({
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_BORDER }
		})
	});

	REGISTER(pipelines::post_processing, PipelineRegistry)<Vertex2D>(
	{
		PipelineDescription::VertexShader(std::nullopt),
		PipelineDescription::PixelShader(std::nullopt),
		PipelineDescription::ComputeShader(shaders::post_processing),
		PipelineDescription::RootSignature(root_signatures::post_processing),
		PipelineDescription::DSVFormat(Format::UNKNOWN),
		PipelineDescription::RTVFormats({ d3d12::settings::back_buffer_format }),
		PipelineDescription::NumRTVFormats(1),
		PipelineDescription::Type(PipelineType::COMPUTE_PIPELINE),
		PipelineDescription::CullMode(CullMode::CULL_NONE),
		PipelineDescription::Depth(false),
		PipelineDescription::CounterClockwise(true),
		PipelineDescription::TopologyType(TopologyType::TRIANGLE)
	});

	REGISTER(pipelines::accumulation, PipelineRegistry)<Vertex2D> (
	{
		PipelineDescription::VertexShader(std::nullopt),
		PipelineDescription::PixelShader(std::nullopt),
		PipelineDescription::ComputeShader(shaders::accumulation),
		PipelineDescription::RootSignature(root_signatures::accumulation),
		PipelineDescription::DSVFormat(Format::UNKNOWN),
		PipelineDescription::RTVFormats({ d3d12::settings::back_buffer_format }),
		PipelineDescription::NumRTVFormats(1),
		PipelineDescription::Type(PipelineType::COMPUTE_PIPELINE),
		PipelineDescription::CullMode(CullMode::CULL_NONE),
		PipelineDescription::Depth(false),
		PipelineDescription::CounterClockwise(true),
		PipelineDescription::TopologyType(TopologyType::TRIANGLE)
	});

	DESC_RANGE_ARRAY(r,
		DESC_RANGE(params::full_raytracing, Type::UAV_RANGE, params::FullRaytracingE::OUTPUT),
		DESC_RANGE(params::full_raytracing, Type::SRV_RANGE, params::FullRaytracingE::INDICES),
		DESC_RANGE(params::full_raytracing, Type::SRV_RANGE, params::FullRaytracingE::LIGHTS),
		DESC_RANGE(params::full_raytracing, Type::SRV_RANGE, params::FullRaytracingE::MATERIALS),
		DESC_RANGE(params::full_raytracing, Type::SRV_RANGE, params::FullRaytracingE::OFFSETS),
		DESC_RANGE(params::full_raytracing, Type::SRV_RANGE, params::FullRaytracingE::SKYBOX),
		DESC_RANGE(params::full_raytracing, Type::SRV_RANGE, params::FullRaytracingE::IRRADIANCE_MAP),
		DESC_RANGE(params::full_raytracing, Type::SRV_RANGE, params::FullRaytracingE::TEXTURES),
		DESC_RANGE_H(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, d3d12::settings::fallback_ptrs_offset),
	);

	REGISTER(root_signatures::rt_test_global, RootSignatureRegistry)({
		RootSignatureDescription::Parameters({
			ROOT_PARAM_DESC_TABLE(r, D3D12_SHADER_VISIBILITY_ALL),
			ROOT_PARAM(GetSRV(params::full_raytracing, params::FullRaytracingE::ACCELERATION_STRUCTURE)),
			ROOT_PARAM(GetCBV(params::full_raytracing, params::FullRaytracingE::CAMERA_PROPERTIES)),
			ROOT_PARAM(GetSRV(params::full_raytracing, params::FullRaytracingE::VERTICES)),
		}),
		RootSignatureDescription::Samplers({
			{ TextureFilter::FILTER_ANISOTROPIC, TextureAddressMode::TAM_WRAP }
		}),
		RootSignatureDescription::ForRTX(true)
	});

	StateObjectDescription::LibraryDesc rt_full_lib = []()
	{
		StateObjectDescription::LibraryDesc lib;
		lib.shader_handle = shaders::rt_lib;
		lib.exports.push_back(L"RaygenEntry");
		lib.exports.push_back(L"ClosestHitEntry");
		lib.exports.push_back(L"MissEntry");
		lib.exports.push_back(L"ShadowClosestHitEntry");
		lib.exports.push_back(L"ShadowMissEntry");
		lib.m_hit_groups.push_back({ L"MyHitGroup", L"ClosestHitEntry" });
		lib.m_hit_groups.push_back({ L"ShadowHitGroup", L"ShadowClosestHitEntry" });

		return lib;
	}();
	
	REGISTER(state_objects::state_object, RTPipelineRegistry)(
	{
		StateObjectDescription::D3D12StateObjectDesc(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE),
		StateObjectDescription::Library(rt_full_lib),
		StateObjectDescription::MaxPayloadSize((sizeof(float)* 7) + sizeof(unsigned int)),
		StateObjectDescription::MaxAttributeSize(sizeof(float)* 4),
		StateObjectDescription::MaxRecursionDepth(3),
		StateObjectDescription::GlobalRootSignature(root_signatures::rt_test_global),
		StateObjectDescription::LocalRootSignatures(std::nullopt),
	});

	/* ### Hybrid Raytracing ### */
	REGISTER(shaders::rt_hybrid_lib, ShaderRegistry)({
		ShaderDescription::Path("resources/shaders/rt_hybrid.hlsl"),
		ShaderDescription::Entry("RaygenEntry"),
		ShaderDescription::Type(ShaderType::LIBRARY_SHADER)
	});

	DESC_RANGE_ARRAY(rt_hybrid_ranges,
		DESC_RANGE(params::rt_hybrid, Type::UAV_RANGE, params::RTHybridE::OUTPUT),
		DESC_RANGE(params::rt_hybrid, Type::SRV_RANGE, params::RTHybridE::INDICES),
		DESC_RANGE(params::rt_hybrid, Type::SRV_RANGE, params::RTHybridE::LIGHTS),
		DESC_RANGE(params::rt_hybrid, Type::SRV_RANGE, params::RTHybridE::MATERIALS),
		DESC_RANGE(params::rt_hybrid, Type::SRV_RANGE, params::RTHybridE::OFFSETS),
		DESC_RANGE(params::rt_hybrid, Type::SRV_RANGE, params::RTHybridE::SKYBOX),
		DESC_RANGE(params::rt_hybrid, Type::SRV_RANGE, params::RTHybridE::IRRADIANCE_MAP),
		DESC_RANGE(params::rt_hybrid, Type::SRV_RANGE, params::RTHybridE::TEXTURES),
		DESC_RANGE(params::rt_hybrid, Type::SRV_RANGE, params::RTHybridE::GBUFFERS),
		DESC_RANGE_H(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 9, d3d12::settings::fallback_ptrs_offset),
	);

	REGISTER(root_signatures::rt_hybrid_global, RootSignatureRegistry)({
		RootSignatureDescription::Parameters({
			ROOT_PARAM_DESC_TABLE(rt_hybrid_ranges, D3D12_SHADER_VISIBILITY_ALL),
			ROOT_PARAM(GetSRV(params::rt_hybrid, params::RTHybridE::ACCELERATION_STRUCTURE)),
			ROOT_PARAM(GetCBV(params::rt_hybrid, params::RTHybridE::CAMERA_PROPERTIES)),
			ROOT_PARAM(GetSRV(params::rt_hybrid, params::RTHybridE::VERTICES)),
		}),
		RootSignatureDescription::Samplers({
			{ TextureFilter::FILTER_ANISOTROPIC, TextureAddressMode::TAM_WRAP }
		}),
		RootSignatureDescription::RTXLocal(true)
	});

	StateObjectDescription::LibraryDesc rt_hybrid_so_library = []()
	{
		StateObjectDescription::LibraryDesc lib;
		lib.shader_handle = shaders::rt_hybrid_lib;
		lib.exports.push_back(L"RaygenEntry");
		lib.exports.push_back(L"ReflectionHit");
		lib.exports.push_back(L"ReflectionMiss");
		lib.exports.push_back(L"ShadowClosestHitEntry");
		lib.exports.push_back(L"ShadowMissEntry");
		lib.m_hit_groups.push_back({L"ReflectionHitGroup", L"ReflectionHit"});
		lib.m_hit_groups.push_back({L"ShadowHitGroup", L"ShadowClosestHitEntry"});

		return lib;
	}();

	REGISTER(state_objects::rt_hybrid_state_object, RTPipelineRegistry)(
	{
		StateObjectDescription::D3D12StateObjectDesc(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE),
		StateObjectDescription::Library(rt_hybrid_so_library),
		StateObjectDescription::MaxPayloadSize((sizeof(float) * 6) + (sizeof(unsigned int) * 1)),
		StateObjectDescription::MaxAttributeSize(sizeof(float) * 4),
		StateObjectDescription::MaxRecursionDepth(3),
		StateObjectDescription::GlobalRootSignature(root_signatures::rt_hybrid_global),
		StateObjectDescription::LocalRootSignatures(std::nullopt),
	});

	/* ### Path Tracer ### */
	REGISTER(shaders::path_tracer_lib, ShaderRegistry)({
		ShaderDescription::Path("resources/shaders/path_tracer.hlsl"),
		ShaderDescription::Entry("RaygenEntry"),
		ShaderDescription::Type(ShaderType::LIBRARY_SHADER)
	});

	StateObjectDescription::LibraryDesc path_tracer_so_library = []()
	{
		StateObjectDescription::LibraryDesc lib;
		lib.shader_handle = shaders::path_tracer_lib;
		lib.exports.push_back(L"RaygenEntry");
		lib.exports.push_back(L"ReflectionHit");
		lib.exports.push_back(L"ReflectionMiss");
		lib.exports.push_back(L"ShadowClosestHitEntry");
		lib.exports.push_back(L"ShadowMissEntry");
		lib.m_hit_groups.push_back({ L"ReflectionHitGroup", L"ReflectionHit" });
		lib.m_hit_groups.push_back({ L"ShadowHitGroup", L"ShadowClosestHitEntry" });

		return lib;
	}();

	REGISTER(state_objects::path_tracer_state_object, RTPipelineRegistry)(
	{
		StateObjectDescription::D3D12StateObjectDesc(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE),
		StateObjectDescription::Library(path_tracer_so_library),
		StateObjectDescription::MaxPayloadSize((sizeof(float) * 7) + (sizeof(unsigned int) * 1)),
		StateObjectDescription::MaxAttributeSize(sizeof(float) * 4),
		StateObjectDescription::MaxRecursionDepth(6),
		StateObjectDescription::GlobalRootSignature(root_signatures::rt_hybrid_global),
		StateObjectDescription::LocalRootSignatures(std::nullopt),
	});

} /* wr */

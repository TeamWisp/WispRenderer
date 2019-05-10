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
// Root Parameter for descriptor tables
#define ROOT_PARAM_DESC_TABLE(arr, visibility) [] { CD3DX12_ROOT_PARAMETER d; d.InitAsDescriptorTable(arr.size(), arr.data(), visibility); return d; }()

namespace wr
{
	using namespace rs_layout;

	//BDRF Lut Precalculation Root Signature
	DESC_RANGE_ARRAY(ranges_brdf,
		DESC_RANGE(params::brdf_lut, Type::UAV_RANGE, params::BRDF_LutE::OUTPUT),
		);

	REGISTER(root_signatures::brdf_lut, RootSignatureRegistry)({
		RootSignatureDescription::Parameters({
			ROOT_PARAM_DESC_TABLE(ranges_brdf, D3D12_SHADER_VISIBILITY_ALL)
		}),
		RootSignatureDescription::Samplers({ })
		});


	//Basic Deferred Pass Root Signature
	DESC_RANGE_ARRAY(ranges_basic,
		DESC_RANGE(params::basic, Type::SRV_RANGE, params::BasicE::ALBEDO),
		DESC_RANGE(params::basic, Type::SRV_RANGE, params::BasicE::NORMAL),
		DESC_RANGE(params::basic, Type::SRV_RANGE, params::BasicE::ROUGHNESS),
		DESC_RANGE(params::basic, Type::SRV_RANGE, params::BasicE::METALLIC), 
		DESC_RANGE(params::basic, Type::SRV_RANGE, params::BasicE::AMBIENT_OCCLUSION),
		DESC_RANGE(params::basic, Type::SRV_RANGE, params::BasicE::EMISSIVE),
	);

	REGISTER(root_signatures::basic, RootSignatureRegistry)({
		RootSignatureDescription::Parameters({
			ROOT_PARAM(GetCBV(params::basic, params::BasicE::CAMERA_PROPERTIES, D3D12_SHADER_VISIBILITY_VERTEX)),
			ROOT_PARAM(GetCBV(params::basic, params::BasicE::OBJECT_PROPERTIES, D3D12_SHADER_VISIBILITY_VERTEX)),
			ROOT_PARAM_DESC_TABLE(ranges_basic, D3D12_SHADER_VISIBILITY_PIXEL),
			ROOT_PARAM(GetCBV(params::basic, params::BasicE::MATERIAL_PROPERTIES, D3D12_SHADER_VISIBILITY_PIXEL)),
		}),
		RootSignatureDescription::Samplers({
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_WRAP }
		})
		});

	//Deferred Composition Root Signature
	DESC_RANGE_ARRAY(srv_ranges,
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::GBUFFER_ALBEDO_ROUGHNESS),
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::GBUFFER_NORMAL_METALLIC),
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::GBUFFER_EMISSIVE_AO),
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::GBUFFER_DEPTH),
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::LIGHT_BUFFER),
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::SKY_BOX),
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::IRRADIANCE_MAP),
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::PREF_ENV_MAP),
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::BRDF_LUT),
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::BUFFER_REFLECTION_SHADOW),
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::BUFFER_SCREEN_SPACE_IRRADIANCE),
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::BUFFER_SCREEN_SPACE_AO),
		DESC_RANGE(params::deferred_composition, Type::UAV_RANGE, params::DeferredCompositionE::OUTPUT),
		);

	REGISTER(root_signatures::deferred_composition, RootSignatureRegistry)({
		RootSignatureDescription::Parameters({
			ROOT_PARAM(GetCBV(params::deferred_composition, params::DeferredCompositionE::CAMERA_PROPERTIES)),
			ROOT_PARAM_DESC_TABLE(srv_ranges, D3D12_SHADER_VISIBILITY_ALL),
		}),
		RootSignatureDescription::Samplers({
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_CLAMP },
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP }
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

	//Prefiltering Root Signature
	DESC_RANGE_ARRAY(prefilter_in_out_ranges,
		DESC_RANGE(params::cubemap_prefiltering, Type::SRV_RANGE, params::CubemapPrefilteringE::SOURCE),
		DESC_RANGE(params::cubemap_prefiltering, Type::UAV_RANGE, params::CubemapPrefilteringE::DEST),
		);
	REGISTER(root_signatures::cubemap_prefiltering, RootSignatureRegistry)({
		RootSignatureDescription::Parameters({
			ROOT_PARAM(GetConstants(params::cubemap_prefiltering, params::CubemapPrefilteringE::CBUFFER)),
			ROOT_PARAM_DESC_TABLE(prefilter_in_out_ranges, D3D12_SHADER_VISIBILITY_ALL)
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


	REGISTER(shaders::brdf_lut_cs, ShaderRegistry)({
		ShaderDescription::Path("resources/shaders/brdf_lut_cs.hlsl"),
		ShaderDescription::Entry("main_cs"),
		ShaderDescription::Type(ShaderType::DIRECT_COMPUTE_SHADER)
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

	REGISTER(shaders::cubemap_prefiltering_cs, ShaderRegistry)({
	ShaderDescription::Path("resources/shaders/prefilter_env_map_cs.hlsl"),
	ShaderDescription::Entry("main_cs"),
	ShaderDescription::Type(ShaderType::DIRECT_COMPUTE_SHADER)
		});

	REGISTER(pipelines::brdf_lut_precalculation, PipelineRegistry) < Vertex2D > ({
		PipelineDescription::VertexShader(std::nullopt),
		PipelineDescription::PixelShader(std::nullopt),
		PipelineDescription::ComputeShader(shaders::brdf_lut_cs),
		PipelineDescription::RootSignature(root_signatures::brdf_lut),
		PipelineDescription::DSVFormat(Format::UNKNOWN),
		PipelineDescription::RTVFormats({ Format::R16G16_FLOAT }),
		PipelineDescription::NumRTVFormats(1),
		PipelineDescription::Type(PipelineType::COMPUTE_PIPELINE),
		PipelineDescription::CullMode(CullMode::CULL_BACK),
		PipelineDescription::Depth(false),
		PipelineDescription::CounterClockwise(true),
		PipelineDescription::TopologyType(TopologyType::TRIANGLE)
		}
	);

	REGISTER(pipelines::basic_deferred, PipelineRegistry) < VertexColor > ({
		PipelineDescription::VertexShader(shaders::basic_vs),
		PipelineDescription::PixelShader(shaders::basic_ps),
		PipelineDescription::ComputeShader(std::nullopt),
		PipelineDescription::RootSignature(root_signatures::basic),
		PipelineDescription::DSVFormat(Format::D32_FLOAT),
		PipelineDescription::RTVFormats({ Format::R16G16B16A16_FLOAT, Format::R16G16B16A16_FLOAT, Format::R8G8B8A8_UNORM }),
		PipelineDescription::NumRTVFormats(3),
		PipelineDescription::Type(PipelineType::GRAPHICS_PIPELINE),
		PipelineDescription::CullMode(CullMode::CULL_NONE),
		PipelineDescription::Depth(true),
		PipelineDescription::CounterClockwise(false),
		PipelineDescription::TopologyType(TopologyType::TRIANGLE)
		});

	REGISTER(pipelines::deferred_composition, PipelineRegistry) < Vertex2D > ({
		PipelineDescription::VertexShader(std::nullopt),
		PipelineDescription::PixelShader(std::nullopt),
		PipelineDescription::ComputeShader(shaders::deferred_composition_cs),
		PipelineDescription::RootSignature(root_signatures::deferred_composition),
		PipelineDescription::DSVFormat(Format::UNKNOWN),
		PipelineDescription::RTVFormats({ wr::Format::R16G16B16A16_FLOAT }),
		PipelineDescription::NumRTVFormats(1),
		PipelineDescription::Type(PipelineType::COMPUTE_PIPELINE),
		PipelineDescription::CullMode(CullMode::CULL_BACK),
		PipelineDescription::Depth(false),
		PipelineDescription::CounterClockwise(true),
		PipelineDescription::TopologyType(TopologyType::TRIANGLE)
		});

	REGISTER(pipelines::mip_mapping, PipelineRegistry) < VertexColor > ({
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

	REGISTER(pipelines::equirect_to_cubemap, PipelineRegistry) < Vertex > (
		{
			PipelineDescription::VertexShader(shaders::equirect_to_cubemap_vs),
			PipelineDescription::PixelShader(shaders::equirect_to_cubemap_ps),
			PipelineDescription::ComputeShader(std::nullopt),
			PipelineDescription::RootSignature(root_signatures::cubemap_conversion),
			PipelineDescription::DSVFormat(Format::UNKNOWN),
			PipelineDescription::RTVFormats({ wr::Format::R16G16B16A16_FLOAT }),
			PipelineDescription::NumRTVFormats(1),
			PipelineDescription::Type(PipelineType::GRAPHICS_PIPELINE),
			PipelineDescription::CullMode(CullMode::CULL_NONE),
			PipelineDescription::Depth(false),
			PipelineDescription::CounterClockwise(false),
			PipelineDescription::TopologyType(TopologyType::TRIANGLE)
		});

	REGISTER(pipelines::cubemap_convolution, PipelineRegistry) < Vertex > (
		{
			PipelineDescription::VertexShader(shaders::equirect_to_cubemap_vs),
			PipelineDescription::PixelShader(shaders::cubemap_convolution_ps),
			PipelineDescription::ComputeShader(std::nullopt),
			PipelineDescription::RootSignature(root_signatures::cubemap_convolution),
			PipelineDescription::DSVFormat(Format::UNKNOWN),
			PipelineDescription::RTVFormats({ wr::Format::R16G16B16A16_FLOAT }),
			PipelineDescription::NumRTVFormats(1),
			PipelineDescription::Type(PipelineType::GRAPHICS_PIPELINE),
			PipelineDescription::CullMode(CullMode::CULL_NONE),
			PipelineDescription::Depth(false),
			PipelineDescription::CounterClockwise(false),
			PipelineDescription::TopologyType(TopologyType::TRIANGLE)
		});

	REGISTER(pipelines::cubemap_prefiltering, PipelineRegistry) < Vertex > (
		{
			PipelineDescription::VertexShader(std::nullopt),
			PipelineDescription::PixelShader(std::nullopt),
			PipelineDescription::ComputeShader(shaders::cubemap_prefiltering_cs),
			PipelineDescription::RootSignature(root_signatures::cubemap_prefiltering),
			PipelineDescription::DSVFormat(Format::UNKNOWN),
			PipelineDescription::RTVFormats({ Format::UNKNOWN }),
			PipelineDescription::NumRTVFormats(0),
			PipelineDescription::Type(PipelineType::COMPUTE_PIPELINE),
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
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_BORDER }
		})
		});

	DESC_RANGE_ARRAY(accum_r,
		DESC_RANGE(params::accumulation, Type::SRV_RANGE, params::AccumulationE::SOURCE),
		DESC_RANGE(params::accumulation, Type::UAV_RANGE, params::AccumulationE::DEST),
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

	REGISTER(pipelines::post_processing, PipelineRegistry) < Vertex2D > (
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

	REGISTER(pipelines::accumulation, PipelineRegistry) < Vertex2D > (
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
		DESC_RANGE(params::full_raytracing, Type::SRV_RANGE, params::FullRaytracingE::BRDF_LUT),
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
			{ TextureFilter::FILTER_ANISOTROPIC, TextureAddressMode::TAM_WRAP },
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_CLAMP }
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
			StateObjectDescription::MaxPayloadSize((sizeof(float) * 7) + sizeof(unsigned int)),
			StateObjectDescription::MaxAttributeSize(sizeof(float) * 4),
			StateObjectDescription::MaxRecursionDepth(3),
			StateObjectDescription::GlobalRootSignature(root_signatures::rt_test_global),
			StateObjectDescription::LocalRootSignatures(std::nullopt),
		});

	/* ### Depth of field ### */

	// Cone of confusion
	REGISTER(shaders::dof_coc, ShaderRegistry) ({
		ShaderDescription::Path("resources/shaders/dof_coc.hlsl"),
			ShaderDescription::Entry("main_cs"),
			ShaderDescription::Type(ShaderType::DIRECT_COMPUTE_SHADER)
		});

	DESC_RANGE_ARRAY(dofcoc_r,
		DESC_RANGE(params::dof_coc, Type::SRV_RANGE, params::DoFCoCE::GDEPTH),
		DESC_RANGE(params::dof_coc, Type::UAV_RANGE, params::DoFCoCE::OUTPUT),
		);

	REGISTER(root_signatures::dof_coc, RootSignatureRegistry)({
		RootSignatureDescription::Parameters({
			ROOT_PARAM_DESC_TABLE(dofcoc_r, D3D12_SHADER_VISIBILITY_ALL),
			ROOT_PARAM(GetCBV(params::dof_coc, params::DoFCoCE::CAMERA_PROPERTIES)),
		}),
		RootSignatureDescription::Samplers({
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_CLAMP}
		})
		});

	REGISTER(pipelines::dof_coc, PipelineRegistry) < Vertex2D > ({
		PipelineDescription::VertexShader(std::nullopt),
		PipelineDescription::PixelShader(std::nullopt),
		PipelineDescription::ComputeShader(shaders::dof_coc),
		PipelineDescription::RootSignature(root_signatures::dof_coc),
		PipelineDescription::DSVFormat(Format::UNKNOWN),
		PipelineDescription::RTVFormats({ Format::R16_FLOAT }),
		PipelineDescription::NumRTVFormats(1),
		PipelineDescription::Type(PipelineType::COMPUTE_PIPELINE),
		PipelineDescription::CullMode(CullMode::CULL_BACK),
		PipelineDescription::Depth(false),
		PipelineDescription::CounterClockwise(true),
		PipelineDescription::TopologyType(TopologyType::TRIANGLE)
		});

	// Down Scale texture
	REGISTER(shaders::down_scale, ShaderRegistry)({
		ShaderDescription::Path("resources/shaders/down_scale.hlsl"),
		ShaderDescription::Entry("main_cs"),
		ShaderDescription::Type(ShaderType::DIRECT_COMPUTE_SHADER)
		});

	DESC_RANGE_ARRAY(dscale_r,
		DESC_RANGE(params::down_scale, Type::SRV_RANGE, params::DownScaleE::SOURCE),
		DESC_RANGE(params::down_scale, Type::UAV_RANGE, params::DownScaleE::OUTPUT_NEAR),
		DESC_RANGE(params::down_scale, Type::UAV_RANGE, params::DownScaleE::OUTPUT_FAR),
		DESC_RANGE(params::down_scale, Type::UAV_RANGE, params::DownScaleE::OUTPUT_BRIGHT),
		DESC_RANGE(params::down_scale, Type::SRV_RANGE, params::DownScaleE::COC),
		);

	REGISTER(root_signatures::down_scale, RootSignatureRegistry)({
		RootSignatureDescription::Parameters({
			ROOT_PARAM_DESC_TABLE(dscale_r, D3D12_SHADER_VISIBILITY_ALL),
		}),
		RootSignatureDescription::Samplers({
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP},
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_CLAMP}
		})
		});

	REGISTER(pipelines::down_scale, PipelineRegistry) < Vertex2D > ({
		PipelineDescription::VertexShader(std::nullopt),
		PipelineDescription::PixelShader(std::nullopt),
		PipelineDescription::ComputeShader(shaders::down_scale),
		PipelineDescription::RootSignature(root_signatures::down_scale),
		PipelineDescription::DSVFormat(Format::UNKNOWN),
		PipelineDescription::RTVFormats({ wr::Format::R16G16B16A16_FLOAT,wr::Format::R16G16B16A16_FLOAT, wr::Format::R16G16B16A16_FLOAT }),
		PipelineDescription::NumRTVFormats(3),
		PipelineDescription::Type(PipelineType::COMPUTE_PIPELINE),
		PipelineDescription::CullMode(CullMode::CULL_BACK),
		PipelineDescription::Depth(false),
		PipelineDescription::CounterClockwise(true),
		PipelineDescription::TopologyType(TopologyType::TRIANGLE)
		});

	// dof dilate
	REGISTER(shaders::dof_dilate, ShaderRegistry)({
		ShaderDescription::Path("resources/shaders/dof_dilate.hlsl"),
		ShaderDescription::Entry("main_cs"),
		ShaderDescription::Type(ShaderType::DIRECT_COMPUTE_SHADER)
		});

	DESC_RANGE_ARRAY(dilate_r,
		DESC_RANGE(params::dof_dilate, Type::SRV_RANGE, params::DoFDilateE::SOURCE),
		DESC_RANGE(params::dof_dilate, Type::UAV_RANGE, params::DoFDilateE::OUTPUT),
		);

	REGISTER(root_signatures::dof_dilate, RootSignatureRegistry)({
		RootSignatureDescription::Parameters({
			ROOT_PARAM_DESC_TABLE(dilate_r, D3D12_SHADER_VISIBILITY_ALL),
		}),
		RootSignatureDescription::Samplers({
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_CLAMP}
		})
		});

	REGISTER(pipelines::dof_dilate, PipelineRegistry) < Vertex2D > ({
		PipelineDescription::VertexShader(std::nullopt),
		PipelineDescription::PixelShader(std::nullopt),
		PipelineDescription::ComputeShader(shaders::dof_dilate),
		PipelineDescription::RootSignature(root_signatures::dof_dilate),
		PipelineDescription::DSVFormat(Format::UNKNOWN),
		PipelineDescription::RTVFormats({ Format::R32_FLOAT }),
		PipelineDescription::NumRTVFormats(1),
		PipelineDescription::Type(PipelineType::COMPUTE_PIPELINE),
		PipelineDescription::CullMode(CullMode::CULL_BACK),
		PipelineDescription::Depth(false),
		PipelineDescription::CounterClockwise(true),
		PipelineDescription::TopologyType(TopologyType::TRIANGLE)
		});

	// dof dilate flatten
	REGISTER(shaders::dof_dilate_flatten, ShaderRegistry)({
		ShaderDescription::Path("resources/shaders/dof_dilate_flatten.hlsl"),
		ShaderDescription::Entry("main_cs"),
		ShaderDescription::Type(ShaderType::DIRECT_COMPUTE_SHADER)
		});

	DESC_RANGE_ARRAY(dilate_flatten_r,
		DESC_RANGE(params::dof_dilate_flatten, Type::SRV_RANGE, params::DoFDilateFlattenE::SOURCE),
		DESC_RANGE(params::dof_dilate_flatten, Type::UAV_RANGE, params::DoFDilateFlattenE::OUTPUT),
		);

	REGISTER(root_signatures::dof_dilate_flatten, RootSignatureRegistry)({
		RootSignatureDescription::Parameters({
			ROOT_PARAM_DESC_TABLE(dilate_flatten_r, D3D12_SHADER_VISIBILITY_ALL),
		}),
		RootSignatureDescription::Samplers({
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_CLAMP}
		})
		});

	REGISTER(pipelines::dof_dilate_flatten, PipelineRegistry) < Vertex2D > ({
		PipelineDescription::VertexShader(std::nullopt),
		PipelineDescription::PixelShader(std::nullopt),
		PipelineDescription::ComputeShader(shaders::dof_dilate_flatten),
		PipelineDescription::RootSignature(root_signatures::dof_dilate_flatten),
		PipelineDescription::DSVFormat(Format::UNKNOWN),
		PipelineDescription::RTVFormats({ Format::R32_FLOAT }),
		PipelineDescription::NumRTVFormats(1),
		PipelineDescription::Type(PipelineType::COMPUTE_PIPELINE),
		PipelineDescription::CullMode(CullMode::CULL_BACK),
		PipelineDescription::Depth(false),
		PipelineDescription::CounterClockwise(true),
		PipelineDescription::TopologyType(TopologyType::TRIANGLE)
		});

	// dof dilate flatten 2nd pass
	REGISTER(shaders::dof_dilate_flatten_h, ShaderRegistry)({
		ShaderDescription::Path("resources/shaders/dof_dilate_flatten_h.hlsl"),
		ShaderDescription::Entry("main_cs"),
		ShaderDescription::Type(ShaderType::DIRECT_COMPUTE_SHADER)
		});

	DESC_RANGE_ARRAY(dilate_flattenh_r,
		DESC_RANGE(params::dof_dilate_flatten_h, Type::SRV_RANGE, params::DoFDilateFlattenHE::SOURCE),
		DESC_RANGE(params::dof_dilate_flatten_h, Type::UAV_RANGE, params::DoFDilateFlattenHE::OUTPUT),
		);

	REGISTER(root_signatures::dof_dilate_flatten_h, RootSignatureRegistry)({
		RootSignatureDescription::Parameters({
			ROOT_PARAM_DESC_TABLE(dilate_flattenh_r, D3D12_SHADER_VISIBILITY_ALL),
		}),
		RootSignatureDescription::Samplers({
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_CLAMP}
		})
		});

	REGISTER(pipelines::dof_dilate_flatten_h, PipelineRegistry) < Vertex2D > ({
		PipelineDescription::VertexShader(std::nullopt),
		PipelineDescription::PixelShader(std::nullopt),
		PipelineDescription::ComputeShader(shaders::dof_dilate_flatten_h),
		PipelineDescription::RootSignature(root_signatures::dof_dilate_flatten_h),
		PipelineDescription::DSVFormat(Format::UNKNOWN),
		PipelineDescription::RTVFormats({ Format::R32_FLOAT }),
		PipelineDescription::NumRTVFormats(1),
		PipelineDescription::Type(PipelineType::COMPUTE_PIPELINE),
		PipelineDescription::CullMode(CullMode::CULL_BACK),
		PipelineDescription::Depth(false),
		PipelineDescription::CounterClockwise(true),
		PipelineDescription::TopologyType(TopologyType::TRIANGLE)
		});

	//dof bokeh
	REGISTER(shaders::dof_bokeh, ShaderRegistry)({
		ShaderDescription::Path("resources/shaders/dof_bokeh.hlsl"),
		ShaderDescription::Entry("main_cs"),
		ShaderDescription::Type(ShaderType::DIRECT_COMPUTE_SHADER)
		});

	DESC_RANGE_ARRAY(dof_bokeh_r,
		DESC_RANGE(params::dof_bokeh, Type::SRV_RANGE, params::DoFBokehE::SOURCE_NEAR),
		DESC_RANGE(params::dof_bokeh, Type::SRV_RANGE, params::DoFBokehE::SOURCE_FAR),
		DESC_RANGE(params::dof_bokeh, Type::UAV_RANGE, params::DoFBokehE::OUTPUT_NEAR),
		DESC_RANGE(params::dof_bokeh, Type::UAV_RANGE, params::DoFBokehE::OUTPUT_FAR),
		DESC_RANGE(params::dof_bokeh, Type::SRV_RANGE, params::DoFBokehE::COC),
		);

	REGISTER(root_signatures::dof_bokeh, RootSignatureRegistry)({
		RootSignatureDescription::Parameters({
			ROOT_PARAM_DESC_TABLE(dof_bokeh_r, D3D12_SHADER_VISIBILITY_ALL),
			ROOT_PARAM(GetCBV(params::dof_bokeh, params::DoFBokehE::CAMERA_PROPERTIES)),
		}),
		RootSignatureDescription::Samplers({
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP},
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_CLAMP}
		})
		});

	REGISTER(pipelines::dof_bokeh, PipelineRegistry) < Vertex2D > ({
		PipelineDescription::VertexShader(std::nullopt),
		PipelineDescription::PixelShader(std::nullopt),
		PipelineDescription::ComputeShader(shaders::dof_bokeh),
		PipelineDescription::RootSignature(root_signatures::dof_bokeh),
		PipelineDescription::DSVFormat(Format::UNKNOWN),
		PipelineDescription::RTVFormats({ wr::Format::R16G16B16A16_FLOAT,wr::Format::R16G16B16A16_FLOAT }),
		PipelineDescription::NumRTVFormats(2),
		PipelineDescription::Type(PipelineType::COMPUTE_PIPELINE),
		PipelineDescription::CullMode(CullMode::CULL_BACK),
		PipelineDescription::Depth(false),
		PipelineDescription::CounterClockwise(true),
		PipelineDescription::TopologyType(TopologyType::TRIANGLE)
		});

	//dof bokeh post filter
	REGISTER(shaders::dof_bokeh_post_filter, ShaderRegistry)({
		ShaderDescription::Path("resources/shaders/dof_bokeh_post_filter.hlsl"),
		ShaderDescription::Entry("main_cs"),
		ShaderDescription::Type(ShaderType::DIRECT_COMPUTE_SHADER)
		});

	DESC_RANGE_ARRAY(dof_bokeh_post_filter_r,
		DESC_RANGE(params::dof_bokeh_post_filter, Type::SRV_RANGE, params::DoFBokehPostFilterE::SOURCE_NEAR),
		DESC_RANGE(params::dof_bokeh_post_filter, Type::SRV_RANGE, params::DoFBokehPostFilterE::SOURCE_FAR),
		DESC_RANGE(params::dof_bokeh_post_filter, Type::UAV_RANGE, params::DoFBokehPostFilterE::OUTPUT_NEAR),
		DESC_RANGE(params::dof_bokeh_post_filter, Type::UAV_RANGE, params::DoFBokehPostFilterE::OUTPUT_FAR),
		);

	REGISTER(root_signatures::dof_bokeh_post_filter, RootSignatureRegistry)({
		RootSignatureDescription::Parameters({
			ROOT_PARAM_DESC_TABLE(dof_bokeh_post_filter_r, D3D12_SHADER_VISIBILITY_ALL),
		}),
		RootSignatureDescription::Samplers({
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP},
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_CLAMP}
		})
		});

	REGISTER(pipelines::dof_bokeh_post_filter, PipelineRegistry) < Vertex2D > ({
		PipelineDescription::VertexShader(std::nullopt),
		PipelineDescription::PixelShader(std::nullopt),
		PipelineDescription::ComputeShader(shaders::dof_bokeh_post_filter),
		PipelineDescription::RootSignature(root_signatures::dof_bokeh_post_filter),
		PipelineDescription::DSVFormat(Format::UNKNOWN),
		PipelineDescription::RTVFormats({ wr::Format::R16G16B16A16_FLOAT, wr::Format::R16G16B16A16_FLOAT }),
		PipelineDescription::NumRTVFormats(2),
		PipelineDescription::Type(PipelineType::COMPUTE_PIPELINE),
		PipelineDescription::CullMode(CullMode::CULL_BACK),
		PipelineDescription::Depth(false),
		PipelineDescription::CounterClockwise(true),
		PipelineDescription::TopologyType(TopologyType::TRIANGLE)
		});

	// depth of field composition
	REGISTER(shaders::dof_composition, ShaderRegistry)({
		ShaderDescription::Path("resources/shaders/dof_composition.hlsl"),
		ShaderDescription::Entry("main_cs"),
		ShaderDescription::Type(ShaderType::DIRECT_COMPUTE_SHADER)
		});

	DESC_RANGE_ARRAY(dof_composition_r,
		DESC_RANGE(params::dof_composition, Type::SRV_RANGE, params::DoFCompositionE::SOURCE),
		DESC_RANGE(params::dof_composition, Type::UAV_RANGE, params::DoFCompositionE::OUTPUT),
		DESC_RANGE(params::dof_composition, Type::SRV_RANGE, params::DoFCompositionE::BOKEH_NEAR),
		DESC_RANGE(params::dof_composition, Type::SRV_RANGE, params::DoFCompositionE::BOKEH_FAR),
		DESC_RANGE(params::dof_composition, Type::SRV_RANGE, params::DoFCompositionE::COC),
		);

	REGISTER(root_signatures::dof_composition, RootSignatureRegistry)({
		RootSignatureDescription::Parameters({
			ROOT_PARAM_DESC_TABLE(dof_composition_r, D3D12_SHADER_VISIBILITY_ALL),
		}),
		RootSignatureDescription::Samplers({
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_CLAMP},
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP}
		})
		});

	REGISTER(pipelines::dof_composition, PipelineRegistry) < Vertex2D > ({
		PipelineDescription::VertexShader(std::nullopt),
		PipelineDescription::PixelShader(std::nullopt),
		PipelineDescription::ComputeShader(shaders::dof_composition),
		PipelineDescription::RootSignature(root_signatures::dof_composition),
		PipelineDescription::DSVFormat(Format::UNKNOWN),
		PipelineDescription::RTVFormats({ wr::Format::R16G16B16A16_FLOAT }),
		PipelineDescription::NumRTVFormats(1),
		PipelineDescription::Type(PipelineType::COMPUTE_PIPELINE),
		PipelineDescription::CullMode(CullMode::CULL_BACK),
		PipelineDescription::Depth(false),
		PipelineDescription::CounterClockwise(true),
		PipelineDescription::TopologyType(TopologyType::TRIANGLE)
		});

	// bloom horizontal blur
	REGISTER(shaders::bloom_h, ShaderRegistry)({
		ShaderDescription::Path("resources/shaders/bloom_horizontal.hlsl"),
		ShaderDescription::Entry("main_cs"),
		ShaderDescription::Type(ShaderType::DIRECT_COMPUTE_SHADER)
		});

	DESC_RANGE_ARRAY(bloom_h_r,
		DESC_RANGE(params::bloom_h, Type::SRV_RANGE, params::BloomHE::SOURCE),
		DESC_RANGE(params::bloom_h, Type::UAV_RANGE, params::BloomHE::OUTPUT),
		);

	REGISTER(root_signatures::bloom_h, RootSignatureRegistry)({
		RootSignatureDescription::Parameters({
			ROOT_PARAM_DESC_TABLE(bloom_h_r, D3D12_SHADER_VISIBILITY_ALL),
		}),
		RootSignatureDescription::Samplers({
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP}
		})
		});

	REGISTER(pipelines::bloom_h, PipelineRegistry) < Vertex2D > ({
		PipelineDescription::VertexShader(std::nullopt),
		PipelineDescription::PixelShader(std::nullopt),
		PipelineDescription::ComputeShader(shaders::bloom_h),
		PipelineDescription::RootSignature(root_signatures::bloom_h),
		PipelineDescription::DSVFormat(Format::UNKNOWN),
		PipelineDescription::RTVFormats({ Format::R16G16B16A16_FLOAT }),
		PipelineDescription::NumRTVFormats(1),
		PipelineDescription::Type(PipelineType::COMPUTE_PIPELINE),
		PipelineDescription::CullMode(CullMode::CULL_BACK),
		PipelineDescription::Depth(false),
		PipelineDescription::CounterClockwise(true),
		PipelineDescription::TopologyType(TopologyType::TRIANGLE)
		});

	// bloom vertical blur
	REGISTER(shaders::bloom_v, ShaderRegistry)({
		ShaderDescription::Path("resources/shaders/bloom_vertical.hlsl"),
		ShaderDescription::Entry("main_cs"),
		ShaderDescription::Type(ShaderType::DIRECT_COMPUTE_SHADER)
		});

	DESC_RANGE_ARRAY(bloom_v_r,
		DESC_RANGE(params::bloom_v, Type::SRV_RANGE, params::BloomVE::SOURCE),
		DESC_RANGE(params::bloom_v, Type::UAV_RANGE, params::BloomVE::OUTPUT),
		);

	REGISTER(root_signatures::bloom_v, RootSignatureRegistry)({
		RootSignatureDescription::Parameters({
			ROOT_PARAM_DESC_TABLE(bloom_v_r, D3D12_SHADER_VISIBILITY_ALL),
		}),
		RootSignatureDescription::Samplers({
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP}
		})
		});

	REGISTER(pipelines::bloom_v, PipelineRegistry) < Vertex2D > ({
		PipelineDescription::VertexShader(std::nullopt),
		PipelineDescription::PixelShader(std::nullopt),
		PipelineDescription::ComputeShader(shaders::bloom_v),
		PipelineDescription::RootSignature(root_signatures::bloom_v),
		PipelineDescription::DSVFormat(Format::UNKNOWN),
		PipelineDescription::RTVFormats({ Format::R16G16B16A16_FLOAT }),
		PipelineDescription::NumRTVFormats(1),
		PipelineDescription::Type(PipelineType::COMPUTE_PIPELINE),
		PipelineDescription::CullMode(CullMode::CULL_BACK),
		PipelineDescription::Depth(false),
		PipelineDescription::CounterClockwise(true),
		PipelineDescription::TopologyType(TopologyType::TRIANGLE)
		});

	// bloom composition
	REGISTER(shaders::bloom_composition, ShaderRegistry)({
		ShaderDescription::Path("resources/shaders/bloom_composition.hlsl"),
		ShaderDescription::Entry("main_cs"),
		ShaderDescription::Type(ShaderType::DIRECT_COMPUTE_SHADER)
		});

	DESC_RANGE_ARRAY(bloom_comp_r,
		DESC_RANGE(params::bloom_composition, Type::SRV_RANGE, params::BloomCompositionE::SOURCE_MAIN),
		DESC_RANGE(params::bloom_composition, Type::SRV_RANGE, params::BloomCompositionE::SOURCE_BLOOM),
		DESC_RANGE(params::bloom_composition, Type::UAV_RANGE, params::BloomCompositionE::OUTPUT),
		);

	REGISTER(root_signatures::bloom_composition, RootSignatureRegistry)({
		RootSignatureDescription::Parameters({
			ROOT_PARAM_DESC_TABLE(bloom_comp_r, D3D12_SHADER_VISIBILITY_ALL),
			ROOT_PARAM(GetConstants(params::bloom_composition, params::BloomCompositionE::BLOOM_PROPERTIES)),
		}),
		RootSignatureDescription::Samplers({
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP},
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_CLAMP}
		})
		});

	REGISTER(pipelines::bloom_composition, PipelineRegistry) < Vertex2D > ({
		PipelineDescription::VertexShader(std::nullopt),
		PipelineDescription::PixelShader(std::nullopt),
		PipelineDescription::ComputeShader(shaders::bloom_composition),
		PipelineDescription::RootSignature(root_signatures::bloom_composition),
		PipelineDescription::DSVFormat(Format::UNKNOWN),
		PipelineDescription::RTVFormats({d3d12::settings::back_buffer_format }),
		PipelineDescription::NumRTVFormats(1),
		PipelineDescription::Type(PipelineType::COMPUTE_PIPELINE),
		PipelineDescription::CullMode(CullMode::CULL_BACK),
		PipelineDescription::Depth(false),
		PipelineDescription::CounterClockwise(true),
		PipelineDescription::TopologyType(TopologyType::TRIANGLE)
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
		DESC_RANGE(params::rt_hybrid, Type::SRV_RANGE, params::RTHybridE::PREF_ENV_MAP),
		DESC_RANGE(params::rt_hybrid, Type::SRV_RANGE, params::RTHybridE::BRDF_LUT),
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
		lib.m_hit_groups.push_back({ L"ReflectionHitGroup", L"ReflectionHit" });
		lib.m_hit_groups.push_back({ L"ShadowHitGroup", L"ShadowClosestHitEntry" });

		return lib;
	}();

	REGISTER(state_objects::rt_hybrid_state_object, RTPipelineRegistry)(
		{
			StateObjectDescription::D3D12StateObjectDesc(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE),
			StateObjectDescription::Library(rt_hybrid_so_library),
			StateObjectDescription::MaxPayloadSize((sizeof(float) * 6) + (sizeof(unsigned int) * 2) + (sizeof(float) * 2)),
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

	DESC_RANGE_ARRAY(path_tracer_ranges,
		DESC_RANGE(params::path_tracing, Type::UAV_RANGE, params::PathTracingE::OUTPUT),
		DESC_RANGE(params::path_tracing, Type::SRV_RANGE, params::PathTracingE::INDICES),
		DESC_RANGE(params::path_tracing, Type::SRV_RANGE, params::PathTracingE::LIGHTS),
		DESC_RANGE(params::path_tracing, Type::SRV_RANGE, params::PathTracingE::MATERIALS),
		DESC_RANGE(params::path_tracing, Type::SRV_RANGE, params::PathTracingE::OFFSETS),
		DESC_RANGE(params::path_tracing, Type::SRV_RANGE, params::PathTracingE::SKYBOX),
		DESC_RANGE(params::path_tracing, Type::SRV_RANGE, params::PathTracingE::PREF_ENV_MAP),
		DESC_RANGE(params::path_tracing, Type::SRV_RANGE, params::PathTracingE::BRDF_LUT),
		DESC_RANGE(params::path_tracing, Type::SRV_RANGE, params::PathTracingE::IRRADIANCE_MAP),
		DESC_RANGE(params::path_tracing, Type::SRV_RANGE, params::PathTracingE::TEXTURES),
		DESC_RANGE(params::path_tracing, Type::SRV_RANGE, params::PathTracingE::GBUFFERS),
		DESC_RANGE_H(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 9, d3d12::settings::fallback_ptrs_offset),
		);

	REGISTER(root_signatures::path_tracing_global, RootSignatureRegistry)({
		RootSignatureDescription::Parameters({
			ROOT_PARAM_DESC_TABLE(path_tracer_ranges, D3D12_SHADER_VISIBILITY_ALL),
			ROOT_PARAM(GetSRV(params::path_tracing, params::PathTracingE::ACCELERATION_STRUCTURE)),
			ROOT_PARAM(GetCBV(params::path_tracing, params::PathTracingE::CAMERA_PROPERTIES)),
			ROOT_PARAM(GetSRV(params::path_tracing, params::PathTracingE::VERTICES)),
		}),
		RootSignatureDescription::Samplers({
			{ TextureFilter::FILTER_ANISOTROPIC, TextureAddressMode::TAM_WRAP }
		}),
		RootSignatureDescription::RTXLocal(true)
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
			StateObjectDescription::GlobalRootSignature(root_signatures::path_tracing_global),
			StateObjectDescription::LocalRootSignatures(std::nullopt),
		});

} /* wr */

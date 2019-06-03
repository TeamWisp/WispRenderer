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
#define DESC_RANGE_H(...) [] { CD3DX12_DESCRIPTOR_RANGE macro_range; macro_range.Init(__VA_ARGS__); return macro_range; }()
// Root parameter
#define ROOT_PARAM(func) [] { return func; }()
// Root Parameter for descriptor tables
#define ROOT_PARAM_DESC_TABLE(arr, visibility) [] { CD3DX12_ROOT_PARAMETER d; d.InitAsDescriptorTable(static_cast<unsigned int>(arr.size()), arr.data(), visibility); return d; }()

namespace wr
{
	using namespace rs_layout;

	//BDRF Lut Precalculation Root Signature
	DESC_RANGE_ARRAY(ranges_brdf,
		DESC_RANGE(params::brdf_lut, Type::UAV_RANGE, params::BRDF_LutE::OUTPUT),
		);

	REGISTER(root_signatures::brdf_lut, RootSignatureRegistry)({
		.m_parameters = {
			ROOT_PARAM_DESC_TABLE(ranges_brdf, D3D12_SHADER_VISIBILITY_ALL)
		},
		.m_samplers = { }
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
		.m_parameters = {
			ROOT_PARAM(GetCBV(params::basic, params::BasicE::CAMERA_PROPERTIES, D3D12_SHADER_VISIBILITY_VERTEX)),
			ROOT_PARAM(GetCBV(params::basic, params::BasicE::OBJECT_PROPERTIES, D3D12_SHADER_VISIBILITY_VERTEX)),
			ROOT_PARAM_DESC_TABLE(ranges_basic, D3D12_SHADER_VISIBILITY_PIXEL),
			ROOT_PARAM(GetCBV(params::basic, params::BasicE::MATERIAL_PROPERTIES, D3D12_SHADER_VISIBILITY_PIXEL)),
		},
		.m_samplers = {
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_WRAP }
		}
	});

	DESC_RANGE_ARRAY(svgf_denoiser_ranges,
		DESC_RANGE(params::svgf_denoiser, Type::SRV_RANGE, params::SVGFDenoiserE::INPUT),
		DESC_RANGE(params::svgf_denoiser, Type::SRV_RANGE, params::SVGFDenoiserE::MOTION),
		DESC_RANGE(params::svgf_denoiser, Type::SRV_RANGE, params::SVGFDenoiserE::NORMAL),
		DESC_RANGE(params::svgf_denoiser, Type::SRV_RANGE, params::SVGFDenoiserE::DEPTH),

		DESC_RANGE(params::svgf_denoiser, Type::SRV_RANGE, params::SVGFDenoiserE::IN_HIST_LENGTH),

		DESC_RANGE(params::svgf_denoiser, Type::SRV_RANGE, params::SVGFDenoiserE::PREV_INPUT),
		DESC_RANGE(params::svgf_denoiser, Type::SRV_RANGE, params::SVGFDenoiserE::PREV_MOMENTS),
		DESC_RANGE(params::svgf_denoiser, Type::SRV_RANGE, params::SVGFDenoiserE::PREV_NORMAL),
		DESC_RANGE(params::svgf_denoiser, Type::SRV_RANGE, params::SVGFDenoiserE::PREV_DEPTH),

		DESC_RANGE(params::svgf_denoiser, Type::UAV_RANGE, params::SVGFDenoiserE::OUT_COLOR),
		DESC_RANGE(params::svgf_denoiser, Type::UAV_RANGE, params::SVGFDenoiserE::OUT_MOMENTS),
		DESC_RANGE(params::svgf_denoiser, Type::UAV_RANGE, params::SVGFDenoiserE::OUT_HIST_LENGTH),
		);

	REGISTER(root_signatures::svgf_denoiser, RootSignatureRegistry)({
		.m_parameters ={
			ROOT_PARAM_DESC_TABLE(svgf_denoiser_ranges, D3D12_SHADER_VISIBILITY_ALL),
			ROOT_PARAM(GetCBV(params::svgf_denoiser, params::SVGFDenoiserE::CAMERA_PROPERTIES)),
			ROOT_PARAM(GetCBV(params::svgf_denoiser, params::SVGFDenoiserE::SVGF_PROPERTIES)),
		},
		.m_samplers = {
			{TextureFilter::FILTER_POINT, TextureAddressMode::TAM_CLAMP},
			{TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP}
		}
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
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::BUFFER_REFLECTION),
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::BUFFER_SHADOW),
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::BUFFER_SCREEN_SPACE_IRRADIANCE),
		DESC_RANGE(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::BUFFER_AO),
		DESC_RANGE(params::deferred_composition, Type::UAV_RANGE, params::DeferredCompositionE::OUTPUT),
	);

	REGISTER(root_signatures::deferred_composition, RootSignatureRegistry)({
		.m_parameters = {
			ROOT_PARAM(GetCBV(params::deferred_composition, params::DeferredCompositionE::CAMERA_PROPERTIES)),
			ROOT_PARAM_DESC_TABLE(srv_ranges, D3D12_SHADER_VISIBILITY_ALL),
		},
		.m_samplers = {
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_CLAMP },
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP }
		}
	});

	//MipMapping Root Signature
	DESC_RANGE_ARRAY(mip_in_out_ranges,
		DESC_RANGE(params::mip_mapping, Type::SRV_RANGE, params::MipMappingE::SOURCE),
		DESC_RANGE(params::mip_mapping, Type::UAV_RANGE, params::MipMappingE::DEST),
	);
	REGISTER(root_signatures::mip_mapping, RootSignatureRegistry)({
		.m_parameters = {
			ROOT_PARAM(GetConstants(params::mip_mapping, params::MipMappingE::CBUFFER)),
			ROOT_PARAM_DESC_TABLE(mip_in_out_ranges, D3D12_SHADER_VISIBILITY_ALL)
		},
		.m_samplers = {
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP }
		}
	});

	//Prefiltering Root Signature
	DESC_RANGE_ARRAY(prefilter_in_out_ranges,
		DESC_RANGE(params::cubemap_prefiltering, Type::SRV_RANGE, params::CubemapPrefilteringE::SOURCE),
		DESC_RANGE(params::cubemap_prefiltering, Type::UAV_RANGE, params::CubemapPrefilteringE::DEST),
		);
	REGISTER(root_signatures::cubemap_prefiltering, RootSignatureRegistry)({
		.m_parameters = {
			ROOT_PARAM(GetConstants(params::cubemap_prefiltering, params::CubemapPrefilteringE::CBUFFER)),
			ROOT_PARAM_DESC_TABLE(prefilter_in_out_ranges, D3D12_SHADER_VISIBILITY_ALL)
		},
		.m_samplers = {
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP }
		}
	});


	//Cubemap conversion root signature
	DESC_RANGE_ARRAY(cubemap_tasks_ranges,
		DESC_RANGE(params::cubemap_conversion, Type::SRV_RANGE, params::CubemapConversionE::EQUIRECTANGULAR_TEXTURE),
		);
	REGISTER(root_signatures::cubemap_conversion, RootSignatureRegistry)({
		.m_parameters = {
			ROOT_PARAM(GetConstants(params::cubemap_conversion, params::CubemapConversionE::IDX)),
			ROOT_PARAM(GetCBV(params::cubemap_conversion, params::CubemapConversionE::CAMERA_PROPERTIES)),
			ROOT_PARAM_DESC_TABLE(cubemap_tasks_ranges, D3D12_SHADER_VISIBILITY_PIXEL),
		},
		.m_samplers = {
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP }
		}
	});

	//Cubemap convolution root signature
	DESC_RANGE_ARRAY(cubemap_convolution_ranges,
		DESC_RANGE(params::cubemap_conversion, Type::SRV_RANGE, params::CubemapConvolutionE::ENVIRONMENT_CUBEMAP),
		);
	REGISTER(root_signatures::cubemap_convolution, RootSignatureRegistry)({
		.m_parameters = {
			ROOT_PARAM(GetConstants(params::cubemap_convolution, params::CubemapConvolutionE::IDX)),
			ROOT_PARAM(GetCBV(params::cubemap_convolution, params::CubemapConvolutionE::CAMERA_PROPERTIES)),
			ROOT_PARAM_DESC_TABLE(cubemap_tasks_ranges, D3D12_SHADER_VISIBILITY_PIXEL),
		},
		.m_samplers = {
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP }
		}
	});


	REGISTER(shaders::brdf_lut_cs, ShaderRegistry)({
		.path = "resources/shaders/brdf_lut_cs.hlsl",
		.entry = "main_cs",
		.type = ShaderType::DIRECT_COMPUTE_SHADER,
		.defines = {}
	});

	REGISTER(shaders::basic_deferred_vs, ShaderRegistry)({
		.path = "resources/shaders/basic.hlsl",
		.entry = "main_vs",
		.type = ShaderType::VERTEX_SHADER,
		.defines = {}
	});

	REGISTER(shaders::basic_deferred_ps, ShaderRegistry)({
		.path = "resources/shaders/basic.hlsl",
		.entry = "main_ps",
		.type = ShaderType::PIXEL_SHADER,
		.defines = {}
	});

	REGISTER(shaders::basic_hybrid_vs, ShaderRegistry)({
		.path = "resources/shaders/basic.hlsl",
		.entry = "main_vs",
		.type = ShaderType::VERTEX_SHADER,
		.defines = {{L"IS_HYBRID", L"1"}}
		});

	REGISTER(shaders::basic_hybrid_ps, ShaderRegistry)({
		.path = "resources/shaders/basic.hlsl",
		.entry = "main_ps",
		.type = ShaderType::PIXEL_SHADER,
		.defines = {{L"IS_HYBRID", L"1"}}
		});

	REGISTER(shaders::fullscreen_quad_vs, ShaderRegistry)({
		.path = "resources/shaders/fullscreen_quad.hlsl",
		.entry = "main_vs",
		.type = ShaderType::VERTEX_SHADER,
		.defines = {}
		});

	REGISTER(shaders::svgf_denoiser_reprojection_cs, ShaderRegistry)({
		.path = "resources/shaders/SVGF.hlsl",
		.entry = "reprojection_cs",
		.type = ShaderType::DIRECT_COMPUTE_SHADER,
		.defines = {}
	});

	REGISTER(shaders::svgf_denoiser_filter_moments_cs, ShaderRegistry)({
		.path = "resources/shaders/SVGF.hlsl",
		.entry = "filter_moments_cs",
		.type = ShaderType::DIRECT_COMPUTE_SHADER,
		.defines = {}
	});

	REGISTER(shaders::svgf_denoiser_wavelet_filter_cs, ShaderRegistry)({
		.path = "resources/shaders/SVGF.hlsl",
		.entry = "wavelet_filter_cs",
		.type = ShaderType::DIRECT_COMPUTE_SHADER,
		.defines = {}
	});

	REGISTER(shaders::deferred_composition_cs, ShaderRegistry)({
		.path = "resources/shaders/deferred_composition.hlsl",
		.entry = "main_cs",
		.type = ShaderType::DIRECT_COMPUTE_SHADER,
		.defines = {}
	});

	REGISTER(shaders::mip_mapping_cs, ShaderRegistry)({
		.path = "resources/shaders/generate_mips_cs.hlsl",
		.entry = "main",
		.type = ShaderType::DIRECT_COMPUTE_SHADER,
		.defines = {}
	});

	REGISTER(shaders::equirect_to_cubemap_vs, ShaderRegistry)({
		.path = "resources/shaders/equirect_to_cubemap_conversion.hlsl",
		.entry = "main_vs",
		.type = ShaderType::VERTEX_SHADER,
		.defines = {}
	});

	REGISTER(shaders::equirect_to_cubemap_ps, ShaderRegistry)({
		.path = "resources/shaders/equirect_to_cubemap_conversion.hlsl",
		.entry = "main_ps",
		.type = ShaderType::PIXEL_SHADER,
		.defines = {}
	});

	REGISTER(shaders::cubemap_convolution_ps, ShaderRegistry)({
		.path = "resources/shaders/cubemap_convolution.hlsl",
		.entry = "main_ps",
		.type = ShaderType::PIXEL_SHADER,
		.defines = {}
	});

	REGISTER(shaders::cubemap_prefiltering_cs, ShaderRegistry)({
		.path = "resources/shaders/prefilter_env_map_cs.hlsl",
		.entry = "main_cs",
		.type = ShaderType::DIRECT_COMPUTE_SHADER,
		.defines = {}
	});

	REGISTER(pipelines::brdf_lut_precalculation, PipelineRegistry) < Vertex2D > ({
		.m_vertex_shader_handle = std::nullopt,
		.m_pixel_shader_handle = std::nullopt,
		.m_compute_shader_handle = shaders::brdf_lut_cs,
		.m_root_signature_handle = root_signatures::brdf_lut,
		.m_dsv_format = Format::UNKNOWN,
		.m_rtv_formats = { Format::R16G16_FLOAT },
		.m_num_rtv_formats = 1,
		.m_type = PipelineType::COMPUTE_PIPELINE,
		.m_cull_mode = CullMode::CULL_BACK,
		.m_depth_enabled = false,
		.m_counter_clockwise = true,
		.m_topology_type = TopologyType::TRIANGLE
	});

	REGISTER(pipelines::basic_deferred, PipelineRegistry) < VertexColor > ({
		.m_vertex_shader_handle = shaders::basic_deferred_vs,
		.m_pixel_shader_handle = shaders::basic_deferred_ps,
		.m_compute_shader_handle = std::nullopt,
		.m_root_signature_handle = root_signatures::basic,
		.m_dsv_format = Format::D32_FLOAT,
		.m_rtv_formats = { Format::R16G16B16A16_FLOAT, Format::R16G16B16A16_FLOAT, Format::R16G16B16A16_FLOAT },
		.m_num_rtv_formats = 3,
		.m_type = PipelineType::GRAPHICS_PIPELINE,
		.m_cull_mode = CullMode::CULL_NONE,
		.m_depth_enabled = true,
		.m_counter_clockwise = false,
		.m_topology_type = TopologyType::TRIANGLE
		});

	REGISTER(pipelines::basic_hybrid, PipelineRegistry) < VertexColor > ({
		.m_vertex_shader_handle = shaders::basic_hybrid_vs,
		.m_pixel_shader_handle = shaders::basic_hybrid_ps,
		.m_compute_shader_handle = std::nullopt,
		.m_root_signature_handle = root_signatures::basic,
		.m_dsv_format = Format::D32_FLOAT,
		.m_rtv_formats = { wr::Format::R16G16B16A16_FLOAT, wr::Format::R16G16B16A16_FLOAT, Format::R16G16B16A16_FLOAT, wr::Format::R16G16B16A16_FLOAT, wr::Format::R32G32B32A32_FLOAT },
		.m_num_rtv_formats = 5,
		.m_type = PipelineType::GRAPHICS_PIPELINE,
		.m_cull_mode = CullMode::CULL_NONE,
		.m_depth_enabled = true,
		.m_counter_clockwise = false,
		.m_topology_type = TopologyType::TRIANGLE
		});

	REGISTER(pipelines::svgf_denoiser_reprojection, PipelineRegistry) < Vertex2D > ({
		.m_vertex_shader_handle = std::nullopt,
		.m_pixel_shader_handle = std::nullopt,
		.m_compute_shader_handle = shaders::svgf_denoiser_reprojection_cs,
		.m_root_signature_handle = root_signatures::svgf_denoiser,
		.m_dsv_format = Format::UNKNOWN,
		.m_rtv_formats = {Format::R16G16B16A16_FLOAT},
		.m_num_rtv_formats = 1,
		.m_type = PipelineType::COMPUTE_PIPELINE,
		.m_cull_mode = CullMode::CULL_BACK,
		.m_depth_enabled = false,
		.m_counter_clockwise = true,
		.m_topology_type = TopologyType::TRIANGLE
	});

	REGISTER(pipelines::svgf_denoiser_filter_moments, PipelineRegistry) < Vertex2D > ({
		.m_vertex_shader_handle = std::nullopt,
		.m_pixel_shader_handle = std::nullopt,
		.m_compute_shader_handle = shaders::svgf_denoiser_filter_moments_cs,
		.m_root_signature_handle = root_signatures::svgf_denoiser,
		.m_dsv_format = Format::UNKNOWN,
		.m_rtv_formats = {Format::R16G16B16A16_FLOAT},
		.m_num_rtv_formats = 1,
		.m_type = PipelineType::COMPUTE_PIPELINE,
		.m_cull_mode = CullMode::CULL_BACK,
		.m_depth_enabled = false,
		.m_counter_clockwise = true,
		.m_topology_type = TopologyType::TRIANGLE
	});

	REGISTER(pipelines::svgf_denoiser_wavelet_filter, PipelineRegistry) < Vertex2D > ({
		.m_vertex_shader_handle = std::nullopt,
		.m_pixel_shader_handle = std::nullopt,
		.m_compute_shader_handle = shaders::svgf_denoiser_wavelet_filter_cs,
		.m_root_signature_handle = root_signatures::svgf_denoiser,
		.m_dsv_format = Format::UNKNOWN,
		.m_rtv_formats = {Format::R16G16B16A16_FLOAT},
		.m_num_rtv_formats = 1,
		.m_type = PipelineType::COMPUTE_PIPELINE,
		.m_cull_mode = CullMode::CULL_BACK,
		.m_depth_enabled = false,
		.m_counter_clockwise = true,
		.m_topology_type = TopologyType::TRIANGLE
	});

	REGISTER(pipelines::deferred_composition, PipelineRegistry)<Vertex2D>({
		.m_vertex_shader_handle = std::nullopt,
		.m_pixel_shader_handle = std::nullopt,
		.m_compute_shader_handle = shaders::deferred_composition_cs,
		.m_root_signature_handle = root_signatures::deferred_composition,
		.m_dsv_format = Format::UNKNOWN,
		.m_rtv_formats = { wr::Format::R16G16B16A16_FLOAT },
		.m_num_rtv_formats = 1,
		.m_type = PipelineType::COMPUTE_PIPELINE,
		.m_cull_mode = CullMode::CULL_BACK,
		.m_depth_enabled = false,
		.m_counter_clockwise = true,
		.m_topology_type = TopologyType::TRIANGLE
		});

	REGISTER(pipelines::mip_mapping, PipelineRegistry) < VertexColor > ({
		.m_vertex_shader_handle = std::nullopt,
		.m_pixel_shader_handle = std::nullopt,
		.m_compute_shader_handle = shaders::mip_mapping_cs,
		.m_root_signature_handle = root_signatures::mip_mapping,
		.m_dsv_format = Format::UNKNOWN,
		.m_rtv_formats = { },
		.m_num_rtv_formats = 0,
		.m_type = PipelineType::COMPUTE_PIPELINE,
		.m_cull_mode = CullMode::CULL_BACK,
		.m_depth_enabled = false,
		.m_counter_clockwise = true,
		.m_topology_type = TopologyType::TRIANGLE
	});

	REGISTER(pipelines::equirect_to_cubemap, PipelineRegistry) < Vertex > (
	{
		.m_vertex_shader_handle = shaders::equirect_to_cubemap_vs,
		.m_pixel_shader_handle = shaders::equirect_to_cubemap_ps,
		.m_compute_shader_handle = std::nullopt,
		.m_root_signature_handle = root_signatures::cubemap_conversion,
		.m_dsv_format = Format::UNKNOWN,
		.m_rtv_formats = { wr::Format::R16G16B16A16_FLOAT },
		.m_num_rtv_formats = 1,
		.m_type = PipelineType::GRAPHICS_PIPELINE,
		.m_cull_mode = CullMode::CULL_NONE,
		.m_depth_enabled = false,
		.m_counter_clockwise = false,
		.m_topology_type = TopologyType::TRIANGLE
	});

	REGISTER(pipelines::cubemap_convolution, PipelineRegistry) < Vertex > (
	{
		.m_vertex_shader_handle = shaders::equirect_to_cubemap_vs,
		.m_pixel_shader_handle = shaders::cubemap_convolution_ps,
		.m_compute_shader_handle = std::nullopt,
		.m_root_signature_handle = root_signatures::cubemap_convolution,
		.m_dsv_format = Format::UNKNOWN,
		.m_rtv_formats = { wr::Format::R16G16B16A16_FLOAT },
		.m_num_rtv_formats = 1,
		.m_type = PipelineType::GRAPHICS_PIPELINE,
		.m_cull_mode = CullMode::CULL_NONE,
		.m_depth_enabled = false,
		.m_counter_clockwise = false,
		.m_topology_type = TopologyType::TRIANGLE
	});

	REGISTER(pipelines::cubemap_prefiltering, PipelineRegistry) < Vertex > (
	{
		.m_vertex_shader_handle = std::nullopt,
		.m_pixel_shader_handle = std::nullopt,
		.m_compute_shader_handle = shaders::cubemap_prefiltering_cs,
		.m_root_signature_handle = root_signatures::cubemap_prefiltering,
		.m_dsv_format = Format::UNKNOWN,
		.m_rtv_formats = { Format::UNKNOWN },
		.m_num_rtv_formats = 0,
		.m_type = PipelineType::COMPUTE_PIPELINE,
		.m_cull_mode = CullMode::CULL_NONE,
		.m_depth_enabled = false,
		.m_counter_clockwise = false,
		.m_topology_type = TopologyType::TRIANGLE
	});

	/* ### Raytracing ### */
	REGISTER(shaders::post_processing, ShaderRegistry)({
		.path = "resources/shaders/post_processing.hlsl",
		.entry = "main",
		.type = ShaderType::DIRECT_COMPUTE_SHADER,
		.defines = {}
	});

	REGISTER(shaders::accumulation, ShaderRegistry)({
		.path = "resources/shaders/accumulation.hlsl",
		.entry = "main",
		.type = ShaderType::DIRECT_COMPUTE_SHADER,
		.defines = {}
	});

	REGISTER(shaders::rt_lib, ShaderRegistry)({
		.path = "resources/shaders/raytracing.hlsl",
		.entry = "RaygenEntry",
		.type = ShaderType::LIBRARY_SHADER,
		.defines = {}
	});

	DESC_RANGE_ARRAY(post_r,
		DESC_RANGE(params::post_processing, Type::SRV_RANGE, params::PostProcessingE::SOURCE),
		DESC_RANGE(params::post_processing, Type::UAV_RANGE, params::PostProcessingE::DEST),
	);

	REGISTER(root_signatures::post_processing, RootSignatureRegistry)({
		.m_parameters = {
			ROOT_PARAM_DESC_TABLE(post_r, D3D12_SHADER_VISIBILITY_ALL),
			ROOT_PARAM(GetConstants(params::post_processing, params::PostProcessingE::HDR_SUPPORT)),
		},
		.m_samplers = {
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_BORDER }
		}
	});

	DESC_RANGE_ARRAY(accum_r,
		DESC_RANGE(params::accumulation, Type::SRV_RANGE, params::AccumulationE::SOURCE),
		DESC_RANGE(params::accumulation, Type::UAV_RANGE, params::AccumulationE::DEST),
	);

	REGISTER(root_signatures::accumulation, RootSignatureRegistry)({
		.m_parameters = {
			ROOT_PARAM_DESC_TABLE(accum_r, D3D12_SHADER_VISIBILITY_ALL),
			ROOT_PARAM(GetConstants(params::accumulation, params::AccumulationE::FRAME_IDX)),
		},
		.m_samplers = {
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_BORDER }
		}
	});

	REGISTER(pipelines::post_processing, PipelineRegistry) < Vertex2D > (
	{
		.m_vertex_shader_handle = std::nullopt,
		.m_pixel_shader_handle = std::nullopt,
		.m_compute_shader_handle = shaders::post_processing,
		.m_root_signature_handle = root_signatures::post_processing,
		.m_dsv_format = Format::UNKNOWN,
		.m_rtv_formats = { d3d12::settings::back_buffer_format },
		.m_num_rtv_formats = 1,
		.m_type = PipelineType::COMPUTE_PIPELINE,
		.m_cull_mode = CullMode::CULL_NONE,
		.m_depth_enabled = false,
		.m_counter_clockwise = true,
		.m_topology_type = TopologyType::TRIANGLE
	});

	REGISTER(pipelines::accumulation, PipelineRegistry) < Vertex2D > (
	{
		.m_vertex_shader_handle = std::nullopt,
		.m_pixel_shader_handle = std::nullopt,
		.m_compute_shader_handle = shaders::accumulation,
		.m_root_signature_handle = root_signatures::accumulation,
		.m_dsv_format = Format::UNKNOWN,
		.m_rtv_formats = { wr::Format::R16G16B16A16_FLOAT },
		.m_num_rtv_formats = 1,
		.m_type = PipelineType::COMPUTE_PIPELINE,
		.m_cull_mode = CullMode::CULL_NONE,
		.m_depth_enabled = false,
		.m_counter_clockwise = true,
		.m_topology_type = TopologyType::TRIANGLE
	});

	DESC_RANGE_ARRAY(r_full_rt,
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
		.m_parameters = {
			ROOT_PARAM_DESC_TABLE(r_full_rt, D3D12_SHADER_VISIBILITY_ALL),
			ROOT_PARAM(GetSRV(params::full_raytracing, params::FullRaytracingE::ACCELERATION_STRUCTURE)),
			ROOT_PARAM(GetCBV(params::full_raytracing, params::FullRaytracingE::CAMERA_PROPERTIES)),
			ROOT_PARAM(GetSRV(params::full_raytracing, params::FullRaytracingE::VERTICES)),
		},
		.m_samplers = {
			{ TextureFilter::FILTER_ANISOTROPIC, TextureAddressMode::TAM_WRAP },
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_CLAMP }
		},
		.m_rtx = true
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
		.desc = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
		.library_desc = rt_full_lib,
		.max_payload_size = (sizeof(float) * 7) + sizeof(unsigned int),
		.max_attributes_size = sizeof(float) * 4,
		.max_recursion_depth = 3,
		.global_root_signature = root_signatures::rt_test_global,
		.local_root_signatures = {},
	});

	/* ### Depth of field ### */

	// Cone of confusion
	REGISTER(shaders::dof_coc, ShaderRegistry) ({
		.path = "resources/shaders/dof_coc.hlsl",
		.entry = "main_cs",
		.type = ShaderType::DIRECT_COMPUTE_SHADER,
		.defines = {}
	});

	DESC_RANGE_ARRAY(dofcoc_r,
		DESC_RANGE(params::dof_coc, Type::SRV_RANGE, params::DoFCoCE::GDEPTH),
		DESC_RANGE(params::dof_coc, Type::UAV_RANGE, params::DoFCoCE::OUTPUT),
	);

	REGISTER(root_signatures::dof_coc, RootSignatureRegistry)({
		.m_parameters = {
			ROOT_PARAM_DESC_TABLE(dofcoc_r, D3D12_SHADER_VISIBILITY_ALL),
			ROOT_PARAM(GetCBV(params::dof_coc, params::DoFCoCE::CAMERA_PROPERTIES)),
		},
		.m_samplers = {
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_CLAMP}
		}
	});

	REGISTER(pipelines::dof_coc, PipelineRegistry) < Vertex2D > ({
		.m_vertex_shader_handle = std::nullopt,
		.m_pixel_shader_handle = std::nullopt,
		.m_compute_shader_handle = shaders::dof_coc,
		.m_root_signature_handle = root_signatures::dof_coc,
		.m_dsv_format = Format::UNKNOWN,
		.m_rtv_formats = { Format::R16G16_FLOAT },
		.m_num_rtv_formats = 1,
		.m_type = PipelineType::COMPUTE_PIPELINE,
		.m_cull_mode = CullMode::CULL_BACK,
		.m_depth_enabled = false,
		.m_counter_clockwise = true,
		.m_topology_type = TopologyType::TRIANGLE
	});

	// Down Scale texture
	REGISTER(shaders::down_scale, ShaderRegistry)({
		.path = "resources/shaders/down_scale.hlsl",
		.entry = "main_cs",
		.type = ShaderType::DIRECT_COMPUTE_SHADER,
		.defines = {}
	});

	DESC_RANGE_ARRAY(dscale_r,
		DESC_RANGE(params::down_scale, Type::SRV_RANGE, params::DownScaleE::SOURCE),
		DESC_RANGE(params::down_scale, Type::UAV_RANGE, params::DownScaleE::OUTPUT_NEAR),
		DESC_RANGE(params::down_scale, Type::UAV_RANGE, params::DownScaleE::OUTPUT_FAR),
		DESC_RANGE(params::down_scale, Type::SRV_RANGE, params::DownScaleE::COC),
	);

	REGISTER(root_signatures::down_scale, RootSignatureRegistry)({
		.m_parameters = {
			ROOT_PARAM_DESC_TABLE(dscale_r, D3D12_SHADER_VISIBILITY_ALL),
		},
		.m_samplers = {
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP},
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_CLAMP}
		}
	});

	REGISTER(pipelines::down_scale, PipelineRegistry) < Vertex2D > ({
		.m_vertex_shader_handle = std::nullopt,
		.m_pixel_shader_handle = std::nullopt,
		.m_compute_shader_handle = shaders::down_scale,
		.m_root_signature_handle = root_signatures::down_scale,
		.m_dsv_format = Format::UNKNOWN,
		.m_rtv_formats = { wr::Format::R16G16B16A16_FLOAT,wr::Format::R16G16B16A16_FLOAT},
		.m_num_rtv_formats = 2,
		.m_type = PipelineType::COMPUTE_PIPELINE,
		.m_cull_mode = CullMode::CULL_BACK,
		.m_depth_enabled = false,
		.m_counter_clockwise = true,
		.m_topology_type = TopologyType::TRIANGLE
	});

	// dof dilate
	REGISTER(shaders::dof_dilate, ShaderRegistry)({
		.path = "resources/shaders/dof_dilate.hlsl",
		.entry = "main_cs",
		.type = ShaderType::DIRECT_COMPUTE_SHADER,
		.defines = {}
	});

	DESC_RANGE_ARRAY(dilate_r,
		DESC_RANGE(params::dof_dilate, Type::SRV_RANGE, params::DoFDilateE::SOURCE),
		DESC_RANGE(params::dof_dilate, Type::UAV_RANGE, params::DoFDilateE::OUTPUT),
	);

	REGISTER(root_signatures::dof_dilate, RootSignatureRegistry)({
		.m_parameters = {
			ROOT_PARAM_DESC_TABLE(dilate_r, D3D12_SHADER_VISIBILITY_ALL),
		},
		.m_samplers = {
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_CLAMP}
		}
	});

	REGISTER(pipelines::dof_dilate, PipelineRegistry) < Vertex2D > ({
		.m_vertex_shader_handle = std::nullopt,
		.m_pixel_shader_handle = std::nullopt,
		.m_compute_shader_handle = shaders::dof_dilate,
		.m_root_signature_handle = root_signatures::dof_dilate,
		.m_dsv_format = Format::UNKNOWN,
		.m_rtv_formats = { Format::R32_FLOAT },
		.m_num_rtv_formats = 1,
		.m_type = PipelineType::COMPUTE_PIPELINE,
		.m_cull_mode = CullMode::CULL_BACK,
		.m_depth_enabled = false,
		.m_counter_clockwise = true,
		.m_topology_type = TopologyType::TRIANGLE
	});

	//dof bokeh
	REGISTER(shaders::dof_bokeh, ShaderRegistry)({
		.path = "resources/shaders/dof_bokeh.hlsl",
		.entry = "main_cs",
		.type = ShaderType::DIRECT_COMPUTE_SHADER,
		.defines = {}
	});

	DESC_RANGE_ARRAY(dof_bokeh_r,
		DESC_RANGE(params::dof_bokeh, Type::SRV_RANGE, params::DoFBokehE::SOURCE_NEAR),
		DESC_RANGE(params::dof_bokeh, Type::SRV_RANGE, params::DoFBokehE::SOURCE_FAR),
		DESC_RANGE(params::dof_bokeh, Type::UAV_RANGE, params::DoFBokehE::OUTPUT_NEAR),
		DESC_RANGE(params::dof_bokeh, Type::UAV_RANGE, params::DoFBokehE::OUTPUT_FAR),
		DESC_RANGE(params::dof_bokeh, Type::SRV_RANGE, params::DoFBokehE::COC),
	);

	REGISTER(root_signatures::dof_bokeh, RootSignatureRegistry)({
		.m_parameters = {
			ROOT_PARAM_DESC_TABLE(dof_bokeh_r, D3D12_SHADER_VISIBILITY_ALL),
			ROOT_PARAM(GetCBV(params::dof_bokeh, params::DoFBokehE::CAMERA_PROPERTIES)),
		},
		.m_samplers = {
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP},
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_CLAMP}
		}
	});

	REGISTER(pipelines::dof_bokeh, PipelineRegistry) < Vertex2D > ({
		.m_vertex_shader_handle = std::nullopt,
		.m_pixel_shader_handle = std::nullopt,
		.m_compute_shader_handle = shaders::dof_bokeh,
		.m_root_signature_handle = root_signatures::dof_bokeh,
		.m_dsv_format = Format::UNKNOWN,
		.m_rtv_formats = { wr::Format::R16G16B16A16_FLOAT,wr::Format::R16G16B16A16_FLOAT },
		.m_num_rtv_formats = 2,
		.m_type = PipelineType::COMPUTE_PIPELINE,
		.m_cull_mode = CullMode::CULL_BACK,
		.m_depth_enabled = false,
		.m_counter_clockwise = true,
		.m_topology_type = TopologyType::TRIANGLE
	});

	//dof bokeh post filter
	REGISTER(shaders::dof_bokeh_post_filter, ShaderRegistry)({
		.path = "resources/shaders/dof_bokeh_post_filter.hlsl",
		.entry = "main_cs",
		.type = ShaderType::DIRECT_COMPUTE_SHADER,
		.defines = {}
	});

	DESC_RANGE_ARRAY(dof_bokeh_post_filter_r,
		DESC_RANGE(params::dof_bokeh_post_filter, Type::SRV_RANGE, params::DoFBokehPostFilterE::SOURCE_NEAR),
		DESC_RANGE(params::dof_bokeh_post_filter, Type::SRV_RANGE, params::DoFBokehPostFilterE::SOURCE_FAR),
		DESC_RANGE(params::dof_bokeh_post_filter, Type::UAV_RANGE, params::DoFBokehPostFilterE::OUTPUT_NEAR),
		DESC_RANGE(params::dof_bokeh_post_filter, Type::UAV_RANGE, params::DoFBokehPostFilterE::OUTPUT_FAR),
	);

	REGISTER(root_signatures::dof_bokeh_post_filter, RootSignatureRegistry)({
		.m_parameters = {
			ROOT_PARAM_DESC_TABLE(dof_bokeh_post_filter_r, D3D12_SHADER_VISIBILITY_ALL),
		},
		.m_samplers = {
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP},
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_CLAMP}
		}
	});

	REGISTER(pipelines::dof_bokeh_post_filter, PipelineRegistry) < Vertex2D > ({
		.m_vertex_shader_handle = std::nullopt,
		.m_pixel_shader_handle = std::nullopt,
		.m_compute_shader_handle = shaders::dof_bokeh_post_filter,
		.m_root_signature_handle = root_signatures::dof_bokeh_post_filter,
		.m_dsv_format = Format::UNKNOWN,
		.m_rtv_formats = { wr::Format::R16G16B16A16_FLOAT, wr::Format::R16G16B16A16_FLOAT },
		.m_num_rtv_formats = 2,
		.m_type = PipelineType::COMPUTE_PIPELINE,
		.m_cull_mode = CullMode::CULL_BACK,
		.m_depth_enabled = false,
		.m_counter_clockwise = true,
		.m_topology_type = TopologyType::TRIANGLE
	});

	// depth of field composition
	REGISTER(shaders::dof_composition, ShaderRegistry)({
		.path = "resources/shaders/dof_composition.hlsl",
		.entry = "main_cs",
		.type = ShaderType::DIRECT_COMPUTE_SHADER,
		.defines = {}
	});

	DESC_RANGE_ARRAY(dof_composition_r,
		DESC_RANGE(params::dof_composition, Type::SRV_RANGE, params::DoFCompositionE::SOURCE),
		DESC_RANGE(params::dof_composition, Type::UAV_RANGE, params::DoFCompositionE::OUTPUT),
		DESC_RANGE(params::dof_composition, Type::SRV_RANGE, params::DoFCompositionE::BOKEH_NEAR),
		DESC_RANGE(params::dof_composition, Type::SRV_RANGE, params::DoFCompositionE::BOKEH_FAR),
		DESC_RANGE(params::dof_composition, Type::SRV_RANGE, params::DoFCompositionE::COC),
	);

	REGISTER(root_signatures::dof_composition, RootSignatureRegistry)({
		.m_parameters = {
			ROOT_PARAM_DESC_TABLE(dof_composition_r, D3D12_SHADER_VISIBILITY_ALL),
		},
		.m_samplers = {
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_CLAMP},
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP}
		}
	});

	REGISTER(pipelines::dof_composition, PipelineRegistry) < Vertex2D > ({
		.m_vertex_shader_handle = std::nullopt,
		.m_pixel_shader_handle = std::nullopt,
		.m_compute_shader_handle = shaders::dof_composition,
		.m_root_signature_handle = root_signatures::dof_composition,
		.m_dsv_format = Format::UNKNOWN,
		.m_rtv_formats = { wr::Format::R16G16B16A16_FLOAT },
		.m_num_rtv_formats = 1,
		.m_type = PipelineType::COMPUTE_PIPELINE,
		.m_cull_mode = CullMode::CULL_BACK,
		.m_depth_enabled = false,
		.m_counter_clockwise = true,
		.m_topology_type = TopologyType::TRIANGLE
	});

	// Extract bright
	REGISTER(shaders::bloom_extract_bright, ShaderRegistry)({
		.path = "resources/shaders/bloom_extract_bright.hlsl",
		.entry = "main_cs",
		.type = ShaderType::DIRECT_COMPUTE_SHADER
		});



	DESC_RANGE_ARRAY(bloombright_r,
		DESC_RANGE(params::bloom_extract_bright, Type::SRV_RANGE, params::BloomExtractBrightE::SOURCE),
		DESC_RANGE(params::bloom_extract_bright, Type::SRV_RANGE, params::BloomExtractBrightE::G_EMISSIVE),
		DESC_RANGE(params::bloom_extract_bright, Type::SRV_RANGE, params::BloomExtractBrightE::G_DEPTH),
		DESC_RANGE(params::bloom_extract_bright, Type::UAV_RANGE, params::BloomExtractBrightE::OUTPUT_BRIGHT),
		);

	REGISTER(root_signatures::bloom_extract_bright, RootSignatureRegistry)({
		.m_parameters = {
			ROOT_PARAM_DESC_TABLE(bloombright_r, D3D12_SHADER_VISIBILITY_ALL),
		},
		.m_samplers = {
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP},
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_CLAMP}
		}
		});

	REGISTER(pipelines::bloom_extract_bright, PipelineRegistry) < Vertex2D > ({
		.m_vertex_shader_handle = std::nullopt,
		.m_pixel_shader_handle = std::nullopt,
		.m_compute_shader_handle = shaders::bloom_extract_bright,
		.m_root_signature_handle = root_signatures::bloom_extract_bright,
		.m_dsv_format = Format::UNKNOWN,
		.m_rtv_formats = { wr::Format::R16G16B16A16_FLOAT },
		.m_num_rtv_formats = 1,
		.m_type = PipelineType::COMPUTE_PIPELINE,
		.m_cull_mode = CullMode::CULL_BACK,
		.m_depth_enabled = false,
		.m_counter_clockwise = true,
		.m_topology_type = TopologyType::TRIANGLE
		});

	// bloom blur
	REGISTER(shaders::bloom_blur, ShaderRegistry)({
		.path = "resources/shaders/bloom_blur.hlsl",
		.entry = "main_cs",
		.type = ShaderType::DIRECT_COMPUTE_SHADER,
		.defines = {}
		});


	DESC_RANGE_ARRAY(bloom_blur_r,
		DESC_RANGE(params::bloom_blur, Type::SRV_RANGE, params::BloomBlurE::SOURCE),
		DESC_RANGE(params::bloom_blur, Type::UAV_RANGE, params::BloomBlurE::OUTPUT),
		);

	REGISTER(root_signatures::bloom_blur, RootSignatureRegistry)({
		.m_parameters = {
			ROOT_PARAM_DESC_TABLE(bloom_blur_r, D3D12_SHADER_VISIBILITY_ALL),
			ROOT_PARAM(GetCBV(params::bloom_blur, params::BloomBlurE::BLUR_DIRECTION)),
		},
		.m_samplers = {
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP},
		}
		});

	REGISTER(pipelines::bloom_blur, PipelineRegistry) < Vertex2D > ({
		.m_vertex_shader_handle = std::nullopt,
		.m_pixel_shader_handle = std::nullopt,
		.m_compute_shader_handle = shaders::bloom_blur,
		.m_root_signature_handle = root_signatures::bloom_blur,
		.m_dsv_format = Format::UNKNOWN,
		.m_rtv_formats = {wr::Format::R16G16B16A16_FLOAT},
		.m_num_rtv_formats = 1,
		.m_type = PipelineType::COMPUTE_PIPELINE,
		.m_cull_mode = CullMode::CULL_BACK,
		.m_depth_enabled = false,
		.m_counter_clockwise = true,
		.m_topology_type = TopologyType::TRIANGLE
		});

	// bloom composition
	REGISTER(shaders::bloom_composition, ShaderRegistry)({
		.path = "resources/shaders/bloom_composition.hlsl",
		.entry = "main_cs",
		.type = ShaderType::DIRECT_COMPUTE_SHADER,
		.defines = {}
	});

	DESC_RANGE_ARRAY(bloom_comp_r,
		DESC_RANGE(params::bloom_composition, Type::SRV_RANGE, params::BloomCompositionE::SOURCE_MAIN),
		DESC_RANGE(params::bloom_composition, Type::SRV_RANGE, params::BloomCompositionE::SOURCE_BLOOM_HALF),
		DESC_RANGE(params::bloom_composition, Type::SRV_RANGE, params::BloomCompositionE::SOURCE_BLOOM_QUARTER),
		DESC_RANGE(params::bloom_composition, Type::SRV_RANGE, params::BloomCompositionE::SOURCE_BLOOM_EIGHTH),
		DESC_RANGE(params::bloom_composition, Type::SRV_RANGE, params::BloomCompositionE::SOURCE_BLOOM_SIXTEENTH),
		DESC_RANGE(params::bloom_composition, Type::UAV_RANGE, params::BloomCompositionE::OUTPUT),
	);

	REGISTER(root_signatures::bloom_composition, RootSignatureRegistry)({
		.m_parameters = {
			ROOT_PARAM_DESC_TABLE(bloom_comp_r, D3D12_SHADER_VISIBILITY_ALL),
			ROOT_PARAM(GetConstants(params::bloom_composition, params::BloomCompositionE::BLOOM_PROPERTIES)),
		},
		.m_samplers = {
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP},
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_CLAMP}
		}
	});

	REGISTER(pipelines::bloom_composition, PipelineRegistry) < Vertex2D > ({
		.m_vertex_shader_handle = std::nullopt,
		.m_pixel_shader_handle = std::nullopt,
		.m_compute_shader_handle = shaders::bloom_composition,
		.m_root_signature_handle = root_signatures::bloom_composition,
		.m_dsv_format = Format::UNKNOWN,
		.m_rtv_formats = {d3d12::settings::back_buffer_format },
		.m_num_rtv_formats = 1,
		.m_type = PipelineType::COMPUTE_PIPELINE,
		.m_cull_mode = CullMode::CULL_BACK,
		.m_depth_enabled = false,
		.m_counter_clockwise = true,
		.m_topology_type = TopologyType::TRIANGLE
	});


	/* ### Hybrid Raytracing ### */
	REGISTER(shaders::rt_hybrid_lib, ShaderRegistry)({
		.path = "resources/shaders/rt_reflection_shadows.hlsl",
		.entry = "HybridRaygenEntry",
		.type = ShaderType::LIBRARY_SHADER,
		.defines = {}
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
		.m_parameters = {
			ROOT_PARAM_DESC_TABLE(rt_hybrid_ranges, D3D12_SHADER_VISIBILITY_ALL),
			ROOT_PARAM(GetSRV(params::rt_hybrid, params::RTHybridE::ACCELERATION_STRUCTURE)),
			ROOT_PARAM(GetCBV(params::rt_hybrid, params::RTHybridE::CAMERA_PROPERTIES)),
			ROOT_PARAM(GetSRV(params::rt_hybrid, params::RTHybridE::VERTICES)),
		},
		.m_samplers = {
			{ TextureFilter::FILTER_ANISOTROPIC, TextureAddressMode::TAM_WRAP }
		},
		.m_rtx = true
	});

	StateObjectDescription::LibraryDesc rt_hybrid_so_library = []()
	{
		StateObjectDescription::LibraryDesc lib;
		lib.shader_handle = shaders::rt_hybrid_lib;
		lib.exports.push_back(L"HybridRaygenEntry");
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
		.desc = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
		.library_desc = rt_hybrid_so_library,
		.max_payload_size = (sizeof(float) * 6) + (sizeof(unsigned int) * 2) + (sizeof(float) * 2),
		.max_attributes_size = sizeof(float) * 4,
		.max_recursion_depth = 3,
		.global_root_signature = root_signatures::rt_hybrid_global,
		.local_root_signatures = {},
	});

	/*### Raytraced Ambient Oclusion ### */
	REGISTER(shaders::rt_ao_lib, ShaderRegistry)({
		.path = "resources/shaders/ambient_occlusion.hlsl",
		.entry = "AORaygenEntry",
		.type = ShaderType::LIBRARY_SHADER,
		.defines = {}
	});

	DESC_RANGE_ARRAY(rt_ao_ranges,
		DESC_RANGE(params::rt_ao, Type::UAV_RANGE, params::RTAOE::OUTPUT),
		DESC_RANGE(params::rt_ao, Type::SRV_RANGE, params::RTAOE::GBUFFERS),
		DESC_RANGE_H(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 9, d3d12::settings::fallback_ptrs_offset),
	);

	REGISTER(root_signatures::rt_ao_global, RootSignatureRegistry)({
		.m_parameters = {
			ROOT_PARAM_DESC_TABLE(rt_ao_ranges, D3D12_SHADER_VISIBILITY_ALL),
			ROOT_PARAM(GetSRV(params::rt_ao, params::RTAOE::ACCELERATION_STRUCTURE)),
			ROOT_PARAM(GetCBV(params::rt_ao, params::RTAOE::CAMERA_PROPERTIES)),
		},
		.m_samplers = {},
		.m_rtx = true
	});

	StateObjectDescription::LibraryDesc rt_ao_so_library = []()
	{
		StateObjectDescription::LibraryDesc lib;
		lib.shader_handle = shaders::rt_ao_lib;
		lib.exports.push_back(L"AORaygenEntry");
		lib.exports.push_back(L"ClosestHitEntry");
		lib.exports.push_back(L"MissEntry");
		lib.m_hit_groups.push_back({ L"AOHitGroup", L"ClosestHitEntry" });
		return lib;
	}();

	REGISTER(state_objects::rt_ao_state_opbject, RTPipelineRegistry)(
	{
		.desc = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
		.library_desc = rt_ao_so_library,
		.max_payload_size =  (sizeof(float) * 2), 
		.max_attributes_size = sizeof(float) * 4,
		.max_recursion_depth = 1,
		.global_root_signature = root_signatures::rt_ao_global,
		.local_root_signatures = {},
	});
	
	/* ### Path Tracer ### */
	REGISTER(shaders::path_tracer_lib, ShaderRegistry)({
		.path = "resources/shaders/path_tracer.hlsl",
		.entry = "RaygenEntry",
		.type = ShaderType::LIBRARY_SHADER,
		.defines = {}
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
		.m_parameters = {
			ROOT_PARAM_DESC_TABLE(path_tracer_ranges, D3D12_SHADER_VISIBILITY_ALL),
			ROOT_PARAM(GetSRV(params::path_tracing, params::PathTracingE::ACCELERATION_STRUCTURE)),
			ROOT_PARAM(GetCBV(params::path_tracing, params::PathTracingE::CAMERA_PROPERTIES)),
			ROOT_PARAM(GetSRV(params::path_tracing, params::PathTracingE::VERTICES)),
		},
		.m_samplers = {
			{ TextureFilter::FILTER_ANISOTROPIC, TextureAddressMode::TAM_WRAP }
		},
		.m_rtx = true
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
		.desc = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
		.library_desc = path_tracer_so_library,
		.max_payload_size = (sizeof(float) * 7) + (sizeof(unsigned int) * 1),
		.max_attributes_size = sizeof(float) * 4,
		.max_recursion_depth = 6,
		.global_root_signature = root_signatures::path_tracing_global,
		.local_root_signatures = {},
	});

	/* ### Shadow Raytracing ### */
	REGISTER(shaders::rt_shadow_lib, ShaderRegistry)({
		.path = "resources/shaders/rt_reflection_shadows.hlsl",
		.entry = "ShadowRaygenEntry",
		.type = ShaderType::LIBRARY_SHADER,
		.defines = {}
		});
	
	StateObjectDescription::LibraryDesc rt_shadow_so_library = []()
	{
		StateObjectDescription::LibraryDesc lib;
		lib.shader_handle = shaders::rt_shadow_lib;
		lib.exports.push_back(L"ShadowRaygenEntry");
		lib.exports.push_back(L"ReflectionHit");
		lib.exports.push_back(L"ReflectionMiss");
		lib.exports.push_back(L"ShadowClosestHitEntry");
		lib.exports.push_back(L"ShadowMissEntry");
		lib.m_hit_groups.push_back({ L"ReflectionHitGroup", L"ReflectionHit" });
		lib.m_hit_groups.push_back({ L"ShadowHitGroup", L"ShadowClosestHitEntry" });

		return lib;
	}();
	REGISTER(state_objects::rt_shadow_state_object, RTPipelineRegistry)(
		{
			.desc = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
			.library_desc = rt_shadow_so_library,
			.max_payload_size = (sizeof(float) * 6) + (sizeof(unsigned int) * 2) + (sizeof(float) * 2),
			.max_attributes_size = sizeof(float) * 4,
			.max_recursion_depth = 1,
			.global_root_signature = root_signatures::rt_hybrid_global,
			.local_root_signatures = {}
		});

	/* ### Reflection Raytracing ### */
	REGISTER(shaders::rt_reflection_lib, ShaderRegistry)({
		.path = "resources/shaders/rt_reflection_shadows.hlsl",
		.entry = "ReflectionRaygenEntry",
		.type = ShaderType::LIBRARY_SHADER,
		.defines = {}
		});

	StateObjectDescription::LibraryDesc rt_reflection_so_library = []()
	{
		StateObjectDescription::LibraryDesc lib;
		lib.shader_handle = shaders::rt_reflection_lib;
		lib.exports.push_back(L"ReflectionRaygenEntry");
		lib.exports.push_back(L"ReflectionHit");
		lib.exports.push_back(L"ReflectionMiss");
		lib.exports.push_back(L"ShadowClosestHitEntry");
		lib.exports.push_back(L"ShadowMissEntry");
		lib.m_hit_groups.push_back({ L"ReflectionHitGroup", L"ReflectionHit" });
		lib.m_hit_groups.push_back({ L"ShadowHitGroup", L"ShadowClosestHitEntry" });

		return lib;
	}();
	REGISTER(state_objects::rt_reflection_state_object, RTPipelineRegistry)(
		{
			.desc = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
			.library_desc = rt_reflection_so_library,
			.max_payload_size = (sizeof(float) * 6) + (sizeof(unsigned int) * 2) + (sizeof(float) * 2),
			.max_attributes_size = sizeof(float) * 4,
			.max_recursion_depth = 3,
			.global_root_signature = root_signatures::rt_hybrid_global,
			.local_root_signatures = {}
		});

} /* wr */

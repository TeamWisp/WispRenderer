#pragma once

#include "engine_registry.hpp"

#include "pipeline_registry.hpp"
#include "root_signature_registry.hpp"
#include "shader_registry.hpp"
#include "rt_pipeline_registry.hpp"
#include "d3d12/d3d12_structs.hpp"

#define REGISTER(type) decltype(type) type

namespace wr
{
	using namespace layout_util;

	//Basic Deferred Pass Root Signature
	std::array<CD3DX12_DESCRIPTOR_RANGE, 4> ranges_basic
	{
		[] { return GetRange(params::basic, Type::SRV_RANGE, params::BasicE::ALBEDO); }(),
		[] { return GetRange(params::basic, Type::SRV_RANGE, params::BasicE::NORMAL); }(),
		[] { return GetRange(params::basic, Type::SRV_RANGE, params::BasicE::ROUGHNESS); }(),
		[] { return GetRange(params::basic, Type::SRV_RANGE, params::BasicE::METALLIC); }(),
	};

	REGISTER(root_signatures::basic) = RootSignatureRegistry::Get().Register({
		{
			[] { return GetCBV(params::basic, params::BasicE::CAMERA_PROPERTIES, D3D12_SHADER_VISIBILITY_VERTEX); }(),
			[] { return GetCBV(params::basic, params::BasicE::OBJECT_PROPERTIES, D3D12_SHADER_VISIBILITY_VERTEX); }(),
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsDescriptorTable(ranges_basic.size(), ranges_basic.data(), D3D12_SHADER_VISIBILITY_PIXEL); return d; }()
		},
		{
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_WRAP }
		}
	});

	//Deferred Composition Root Signature
	std::array<CD3DX12_DESCRIPTOR_RANGE, 7> srv_ranges
	{
		[] { return GetRange(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::GBUFFER_ALBEDO_ROUGHNESS); }(),
		[] { return GetRange(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::GBUFFER_NORMAL_METALLIC); }(),
		[] { return GetRange(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::GBUFFER_DEPTH); }(),
		[] { return GetRange(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::LIGHT_BUFFER); }(),
		[] { return GetRange(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::SKY_BOX); }(),
		[] { return GetRange(params::deferred_composition, Type::SRV_RANGE, params::DeferredCompositionE::IRRADIANCE_MAP); }(),
		[] { return GetRange(params::deferred_composition, Type::UAV_RANGE, params::DeferredCompositionE::OUTPUT); }(),
	};

	REGISTER(root_signatures::deferred_composition) = RootSignatureRegistry::Get().Register({
		{
			[] { return GetCBV(params::deferred_composition, params::DeferredCompositionE::CAMERA_PROPERTIES); }(),
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsDescriptorTable(srv_ranges.size(), srv_ranges.data(), D3D12_SHADER_VISIBILITY_ALL); return d; }(),
		},
		{
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_CLAMP }
		}
		});


	//MipMapping Root Signature
	std::array< CD3DX12_DESCRIPTOR_RANGE, 2> mip_in_out_ranges
	{
		[] { return GetRange(params::mip_mapping, Type::SRV_RANGE, params::MipMappingE::SOURCE); }(),
		[] { return GetRange(params::mip_mapping, Type::UAV_RANGE, params::MipMappingE::DEST); }(),
	};
	REGISTER(root_signatures::mip_mapping) = RootSignatureRegistry::Get().Register({
		{
			[] { return GetConstants(params::mip_mapping, params::MipMappingE::TEXEL_SIZE); }(),
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsDescriptorTable(mip_in_out_ranges.size(), mip_in_out_ranges.data()); return d; }()
		},
		{
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP }
		}
	});


	//Cubemap conversion root signature
	std::array<CD3DX12_DESCRIPTOR_RANGE, 1> cubemap_tasks_ranges
	{
		[] { return GetRange(params::cubemap_conversion, Type::SRV_RANGE, params::CubemapConversionE::EQUIRECTANGULAR_TEXTURE); }(),
	};
	REGISTER(root_signatures::cubemap_conversion) = RootSignatureRegistry::Get().Register({
		{
			[] { return GetConstants(params::cubemap_conversion, params::CubemapConversionE::IDX); }(),
			[] { return GetCBV(params::cubemap_conversion, params::CubemapConversionE::CAMERA_PROPERTIES); }(),
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsDescriptorTable(cubemap_tasks_ranges.size(), cubemap_tasks_ranges.data(), D3D12_SHADER_VISIBILITY_PIXEL); return d; }()
		},
		{
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP }
		}
	});
	
	//Cubemap convolution root signature
	std::array< CD3DX12_DESCRIPTOR_RANGE, 1> cubemap_convolution_ranges
	{
		[] { return GetRange(params::cubemap_conversion, Type::SRV_RANGE, params::CubemapConvolutionE::ENVIRONMENT_CUBEMAP); }(),
	};
	REGISTER(root_signatures::cubemap_convolution) = RootSignatureRegistry::Get().Register({
		{
			[] { return GetConstants(params::cubemap_convolution, params::CubemapConvolutionE::IDX); }(),
			[] { return GetCBV(params::cubemap_convolution, params::CubemapConvolutionE::CAMERA_PROPERTIES); }(),
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsDescriptorTable(cubemap_tasks_ranges.size(), cubemap_tasks_ranges.data(), D3D12_SHADER_VISIBILITY_PIXEL); return d; }()
		},
		{
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP }
		}
	});


	REGISTER(shaders::basic_vs) = ShaderRegistry::Get().Register({
		"resources/shaders/basic.hlsl",
		"main_vs",
		ShaderType::VERTEX_SHADER
	});

	REGISTER(shaders::basic_ps) = ShaderRegistry::Get().Register({
		"resources/shaders/basic.hlsl",
		"main_ps",
		ShaderType::PIXEL_SHADER
	});

	REGISTER(shaders::fullscreen_quad_vs) = ShaderRegistry::Get().Register({
		"resources/shaders/fullscreen_quad.hlsl",
		"main_vs",
		ShaderType::VERTEX_SHADER
	});

	REGISTER(shaders::deferred_composition_cs) = ShaderRegistry::Get().Register({
		"resources/shaders/deferred_composition.hlsl",
		"main_cs",
		ShaderType::DIRECT_COMPUTE_SHADER
	});

	REGISTER(shaders::mip_mapping_cs) = ShaderRegistry::Get().Register(
	{
		"resources/shaders/generate_mips_cs.hlsl",
		"main",
		ShaderType::DIRECT_COMPUTE_SHADER
	});

	REGISTER(shaders::equirect_to_cubemap_vs) = ShaderRegistry::Get().Register({
		"resources/shaders/equirect_to_cubemap_conversion.hlsl",
		"main_vs",
		ShaderType::VERTEX_SHADER
	});

	REGISTER(shaders::equirect_to_cubemap_ps) = ShaderRegistry::Get().Register({
		"resources/shaders/equirect_to_cubemap_conversion.hlsl",
		"main_ps",
		ShaderType::PIXEL_SHADER
	});

	REGISTER(shaders::cubemap_convolution_ps) = ShaderRegistry::Get().Register({
		"resources/shaders/cubemap_convolution.hlsl",
		"main_ps",
		ShaderType::PIXEL_SHADER
	});


	REGISTER(pipelines::basic_deferred) = PipelineRegistry::Get().Register<VertexColor>({
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

	REGISTER(pipelines::deferred_composition) = PipelineRegistry::Get().Register<Vertex2D>({
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

	REGISTER(pipelines::mip_mapping) = PipelineRegistry::Get().Register<VertexColor>(
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

	REGISTER(pipelines::equirect_to_cubemap) = PipelineRegistry::Get().Register<Vertex>(
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

	REGISTER(pipelines::cubemap_convolution) = PipelineRegistry::Get().Register<Vertex>(
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
	REGISTER(shaders::post_processing) = ShaderRegistry::Get().Register(
		{
			"resources/shaders/post_processing.hlsl",
			"main",
			ShaderType::DIRECT_COMPUTE_SHADER
		});

	REGISTER(shaders::rt_lib) = ShaderRegistry::Get().Register({
		"resources/shaders/raytracing.hlsl",
		"RaygenEntry",
		ShaderType::LIBRARY_SHADER
	});

	std::vector<CD3DX12_DESCRIPTOR_RANGE> accum_r = {
		[] { return GetRange(params::post_processing, Type::SRV_RANGE, params::PostProcessingE::SOURCE); }(),
		[] { return GetRange(params::post_processing, Type::UAV_RANGE, params::PostProcessingE::DEST); }(),
	};

	REGISTER(root_signatures::post_processing) = RootSignatureRegistry::Get().Register({
	{
		[] { CD3DX12_ROOT_PARAMETER d; d.InitAsDescriptorTable(accum_r.size(), accum_r.data()); return d; }(),
		[] { return GetConstants(params::post_processing, params::PostProcessingE::HDR_SUPPORT); }(),
	},
	{
		{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_BORDER }
	}
	});

	REGISTER(pipelines::post_processing) = PipelineRegistry::Get().Register<Vertex2D>(
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

	std::vector<CD3DX12_DESCRIPTOR_RANGE> r = {
		[] { return GetRange(params::full_raytracing, Type::UAV_RANGE, params::FullRaytracingE::OUTPUT); }(),
		[] { return GetRange(params::full_raytracing, Type::SRV_RANGE, params::FullRaytracingE::INDICES); }(),
		[] { return GetRange(params::full_raytracing, Type::SRV_RANGE, params::FullRaytracingE::LIGHTS); }(),
		[] { return GetRange(params::full_raytracing, Type::SRV_RANGE, params::FullRaytracingE::MATERIALS); }(),
		[] { return GetRange(params::full_raytracing, Type::SRV_RANGE, params::FullRaytracingE::OFFSETS); }(),
		[] { return GetRange(params::full_raytracing, Type::SRV_RANGE, params::FullRaytracingE::SKYBOX); }(),
		[] { return GetRange(params::full_raytracing, Type::SRV_RANGE, params::FullRaytracingE::TEXTURES); }(),
		[] { CD3DX12_DESCRIPTOR_RANGE r; r.Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, d3d12::settings::fallback_ptrs_offset); return r; }(), // fallback pointers
	};

	REGISTER(root_signatures::rt_test_global) = RootSignatureRegistry::Get().Register({
		{
			[&] { CD3DX12_ROOT_PARAMETER d; d.InitAsDescriptorTable(r.size(), r.data()); return d; }(),
			[] { return GetSRV(params::full_raytracing, params::FullRaytracingE::ACCELERATION_STRUCTURE); }(),
			[] { return GetCBV(params::full_raytracing, params::FullRaytracingE::CAMERA_PROPERTIES); }(),
			[] { return GetSRV(params::full_raytracing, params::FullRaytracingE::VERTICES); }(),
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
	
	REGISTER(state_objects::state_object) = RTPipelineRegistry::Get().Register(
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
	REGISTER(shaders::rt_hybrid_lib) = ShaderRegistry::Get().Register({
		"resources/shaders/rt_hybrid.hlsl",
		"RaygenEntry",
		ShaderType::LIBRARY_SHADER
	});

	std::vector<CD3DX12_DESCRIPTOR_RANGE> rt_hybrid_ranges = {
		[] { return GetRange(params::rt_hybrid, Type::UAV_RANGE, params::RTHybridE::OUTPUT); }(),
		[] { return GetRange(params::rt_hybrid, Type::SRV_RANGE, params::RTHybridE::INDICES); }(),
		[] { return GetRange(params::rt_hybrid, Type::SRV_RANGE, params::RTHybridE::LIGHTS); }(),
		[] { return GetRange(params::rt_hybrid, Type::SRV_RANGE, params::RTHybridE::MATERIALS); }(),
		[] { return GetRange(params::rt_hybrid, Type::SRV_RANGE, params::RTHybridE::OFFSETS); }(),
		[] { return GetRange(params::rt_hybrid, Type::SRV_RANGE, params::RTHybridE::SKYBOX); }(),
		[] { return GetRange(params::rt_hybrid, Type::SRV_RANGE, params::RTHybridE::TEXTURES); }(),
		[] { return GetRange(params::rt_hybrid, Type::SRV_RANGE, params::RTHybridE::GBUFFERS); }(),
		[] { CD3DX12_DESCRIPTOR_RANGE r; r.Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, d3d12::settings::fallback_ptrs_offset); return r; }(), // fallback pointers
	};

	REGISTER(root_signatures::rt_hybrid_global) = RootSignatureRegistry::Get().Register({
		{
			[&] { CD3DX12_ROOT_PARAMETER d; d.InitAsDescriptorTable(rt_hybrid_ranges.size(), rt_hybrid_ranges.data()); return d; }(),
			[] { return GetSRV(params::rt_hybrid, params::RTHybridE::ACCELERATION_STRUCTURE); }(),
			[] { return GetCBV(params::rt_hybrid, params::RTHybridE::CAMERA_PROPERTIES); }(),
			[] { return GetSRV(params::rt_hybrid, params::RTHybridE::VERTICES); }(),
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

	REGISTER(state_objects::rt_hybrid_state_object) = RTPipelineRegistry::Get().Register(
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
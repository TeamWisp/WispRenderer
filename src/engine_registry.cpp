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
	//Basic Deferred Pass Root Signature
	std::array<CD3DX12_DESCRIPTOR_RANGE, 1> ranges_basic
	{
		[] { CD3DX12_DESCRIPTOR_RANGE r; r.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0); return r; }(),
	};
	REGISTER(root_signatures::basic) = RootSignatureRegistry::Get().Register({
		{
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX); return d; }(),
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX); return d; }(),
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsDescriptorTable(ranges_basic.size(), ranges_basic.data(), D3D12_SHADER_VISIBILITY_PIXEL); return d; }()
		},
		{
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP }
		}
	});

	//Deferred Composition Root Signature
	std::array<CD3DX12_DESCRIPTOR_RANGE, 1> srv_ranges
	{ 
		[] { CD3DX12_DESCRIPTOR_RANGE r; r.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0); return r; }(),
		
	};
	std::array<CD3DX12_DESCRIPTOR_RANGE, 1> uav_ranges
	{
		[] {CD3DX12_DESCRIPTOR_RANGE r; r.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0); return r; }(),
	};
	REGISTER(root_signatures::deferred_composition) = RootSignatureRegistry::Get().Register({
		{
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL); return d; }(),
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsDescriptorTable(srv_ranges.size(), srv_ranges.data(), D3D12_SHADER_VISIBILITY_ALL); return d; }(),
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsDescriptorTable(uav_ranges.size(), uav_ranges.data(), D3D12_SHADER_VISIBILITY_ALL); return d; }(),
		},
		{
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_CLAMP }
		}
		});


	//MipMapping Root Signature
	std::array< CD3DX12_DESCRIPTOR_RANGE, 2> mip_in_out_ranges
	{
		[] { CD3DX12_DESCRIPTOR_RANGE r; r.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0); return r; } (),
		[] { CD3DX12_DESCRIPTOR_RANGE r; r.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0); return r; } ()
	};
	REGISTER(root_signatures::mip_mapping) = RootSignatureRegistry::Get().Register({
		{
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsConstants(2, 0); return d; }(),
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsDescriptorTable(mip_in_out_ranges.size(), mip_in_out_ranges.data()); return d; }()
		},
		{
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP }
		}
	});


	//Cubemap conversion root signature
	std::array<CD3DX12_DESCRIPTOR_RANGE, 1> cubemap_tasks_ranges
	{
		[] { CD3DX12_DESCRIPTOR_RANGE r; r.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); return r; }(),
	};
	REGISTER(root_signatures::cubemap_conversion) = RootSignatureRegistry::Get().Register({
		{
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsConstants(1, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX); return d; }(),
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX); return d; }(),
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsDescriptorTable(cubemap_tasks_ranges.size(), cubemap_tasks_ranges.data(), D3D12_SHADER_VISIBILITY_PIXEL); return d; }()
		},
		{
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP }
		}
	});
	
	//Cubemap convolution root signature
	std::array< CD3DX12_DESCRIPTOR_RANGE, 1> cubemap_convolution_ranges
	{
		[] { CD3DX12_DESCRIPTOR_RANGE r; r.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0); return r; } (),
	};
	REGISTER(root_signatures::cubemap_convolution) = RootSignatureRegistry::Get().Register({
		{
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsConstants(1, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX); return d; }(),
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX); return d; }(),
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


	REGISTER(pipelines::basic_deferred) = PipelineRegistry::Get().Register<Vertex>({
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

	REGISTER(pipelines::mip_mapping) = PipelineRegistry::Get().Register<Vertex>(
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
	REGISTER(shaders::accumulation) = ShaderRegistry::Get().Register(
		{
			"resources/shaders/accumulation.hlsl",
			"main",
			ShaderType::PIXEL_SHADER
		});

	REGISTER(shaders::rt_lib) = ShaderRegistry::Get().Register({
		"resources/shaders/raytracing.hlsl",
		"RaygenEntry",
		ShaderType::LIBRARY_SHADER
	});

	std::vector<CD3DX12_DESCRIPTOR_RANGE> accum_r = {
		[] { CD3DX12_DESCRIPTOR_RANGE r; r.Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); return r; }(), // source texture
	};

	REGISTER(root_signatures::accumulation) = RootSignatureRegistry::Get().Register({
	{
		[] { CD3DX12_ROOT_PARAMETER d; d.InitAsDescriptorTable(accum_r.size(), accum_r.data()); return d; }(),
		[] { CD3DX12_ROOT_PARAMETER d; d.InitAsConstantBufferView(0); return d; }(), // RT Camera
	},
	{
		{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_BORDER }
	}
	});

	REGISTER(pipelines::accumulation) = PipelineRegistry::Get().Register<Vertex2D>(
		{
			shaders::fullscreen_quad_vs,
			shaders::accumulation,
			std::nullopt,
			root_signatures::accumulation,
			Format::UNKNOWN,
			{ Format::R8G8B8A8_UNORM }, //This compute shader doesn't use any render target
			1,
			PipelineType::GRAPHICS_PIPELINE,
			CullMode::CULL_BACK,
			false,
			true,
			TopologyType::TRIANGLE
		});

	std::vector<CD3DX12_DESCRIPTOR_RANGE> r = {
		[] { CD3DX12_DESCRIPTOR_RANGE r; r.Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0); return r; }(), // output texture
		[] { CD3DX12_DESCRIPTOR_RANGE r; r.Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1); return r; }(), // indices and lights
		[] { CD3DX12_DESCRIPTOR_RANGE r; r.Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1+20, 4); return r; }(), // Materials (1) Textures (+20) 
		[] { CD3DX12_DESCRIPTOR_RANGE r; r.Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, d3d12::settings::fallback_ptrs_offset); return r; }(), // Materials (1) Textures (+20) 
	};

	REGISTER(root_signatures::rt_test_global) = RootSignatureRegistry::Get().Register({
		{
			[&] { CD3DX12_ROOT_PARAMETER d; d.InitAsDescriptorTable(r.size(), r.data()); return d; }(),
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsShaderResourceView(0); return d; }(), // Acceleration Structure
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsConstantBufferView(0); return d; }(), // RT Camera
			//[] { CD3DX12_ROOT_PARAMETER d; d.InitAsShaderResourceView(1); return d; }(), // Indices
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsShaderResourceView(3); return d; }(), // Vertices
		},
		{
			{ TextureFilter::FILTER_ANISOTROPIC, TextureAddressMode::TAM_WRAP }
		},
		true // rtx
	});

	REGISTER(root_signatures::rt_test_local) = RootSignatureRegistry::Get().Register({
		{
		},
		{
			// No samplers
		},
		true, true // rtx and local
	});

	std::pair<CD3DX12_STATE_OBJECT_DESC, StateObjectDescription::Library> so_desc = []()
	{
		CD3DX12_STATE_OBJECT_DESC desc = { D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

		StateObjectDescription::Library lib;
		lib.shader_handle = shaders::rt_lib;
		lib.exports.push_back(L"RaygenEntry");
		lib.exports.push_back(L"ClosestHitEntry");
		lib.exports.push_back(L"MissEntry");

		return std::make_pair(desc, lib);
	}();
	
	REGISTER(state_objects::state_object) = RTPipelineRegistry::Get().Register(
	{
		so_desc.first,     // Description
		so_desc.second,    // Library
		(sizeof(float)*7 + sizeof(unsigned int)), // Max payload size
		(sizeof(float)*2), // Max attributes size
		3,				   // Max recursion depth
		root_signatures::rt_test_global,      // Global root signature
		std::vector<RegistryHandle>{ root_signatures::rt_test_local },      // Local Root Signatures
	});

} /* wr */
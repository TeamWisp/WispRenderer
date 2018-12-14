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
		[] { CD3DX12_DESCRIPTOR_RANGE r; r.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0); return r; }(),
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
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_BORDER }
		}
		});

	//MipMapping Root Signature
	std::array< CD3DX12_DESCRIPTOR_RANGE, 1> mip_in_srv_ranges
	{
		[] { CD3DX12_DESCRIPTOR_RANGE r; r.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0); return r; } ()
	};
	std::array< CD3DX12_DESCRIPTOR_RANGE, 1> mip_out_uav_ranges
	{
		[] { CD3DX12_DESCRIPTOR_RANGE r; r.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 4, 0, 0); return r; } ()
	};
	REGISTER(root_signatures::mip_mapping) = RootSignatureRegistry::Get().Register({
		{
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsConstants(2, 0); return d; }(),
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsDescriptorTable(mip_in_srv_ranges.size(), mip_in_srv_ranges.data()); return d; }(),
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsDescriptorTable(mip_out_uav_ranges.size(), mip_out_uav_ranges.data()); return d; }()
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

	/* ### Raytracing ### */
	REGISTER(shaders::rt_lib) = ShaderRegistry::Get().Register({
		"resources/shaders/raytracing.hlsl",
		"RaygenEntry",
		ShaderType::LIBRARY_SHADER
	});

	D3D12_DESCRIPTOR_RANGE r[1] = {
		//{ D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, 0 },
		{ D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, 0 }
	};

	REGISTER(root_signatures::rt_test_global) = RootSignatureRegistry::Get().Register({
		{
			[&] { CD3DX12_ROOT_PARAMETER d; d.InitAsDescriptorTable(1, r); return d; }(),
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsShaderResourceView(0); return d; }(),
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsConstantBufferView(0); return d; }(),
		},
		{
			// No samplers
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
		(sizeof(float)*4), // Max payload size
		(sizeof(float)*2), // Max attributes size
		1,				   // Max recursion depth
		root_signatures::rt_test_global,      // Global root signature
		std::vector<RegistryHandle>{ root_signatures::rt_test_local },      // Local Root Signatures
	});

} /* wr */
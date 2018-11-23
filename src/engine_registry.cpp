#pragma once

#include "engine_registry.hpp"

#include "pipeline_registry.hpp"
#include "root_signature_registry.hpp"
#include "shader_registry.hpp"

#define REGISTER(type) decltype(type) type

namespace wr
{

	REGISTER(root_signatures::basic) = RootSignatureRegistry::Get().Register({
		{
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX); return d; }(),
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX); return d; }(),
		},
		{
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_MIRROR }
		}
	});

	REGISTER(root_signatures::lighting) = RootSignatureRegistry::Get().Register({
		{
		},
		{ 
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_CLAMP } 
		},
		{
			{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0 }
		}
	});

	REGISTER(shaders::basic_vs) = ShaderRegistry::Get().Register({
		"basic.hlsl",
		"main_vs",
		ShaderType::VERTEX_SHADER
	});

	REGISTER(shaders::basic_ps) = ShaderRegistry::Get().Register({
		"basic.hlsl",
		"main_ps",
		ShaderType::PIXEL_SHADER
	});

	REGISTER(shaders::lighting_vs) = ShaderRegistry::Get().Register({
		"lighting.hlsl",
		"main_vs",
		ShaderType::VERTEX_SHADER
	});

	REGISTER(shaders::lighting_ps) = ShaderRegistry::Get().Register({
		"lighting.hlsl",
		"main_ps",
		ShaderType::PIXEL_SHADER
	});

	REGISTER(pipelines::basic) = PipelineRegistry::Get().Register<Vertex>({
		shaders::basic_vs,
		shaders::basic_ps,
		std::nullopt,
		root_signatures::basic,
		Format::D32_FLOAT,
		{ Format::R16G16B16A16_FLOAT, Format::R8G8B8A8_SNORM },
		2,
		PipelineType::GRAPHICS_PIPELINE,
		CullMode::CULL_BACK,
		true,
		true,
		TopologyType::TRIANGLE
	});

	REGISTER(pipelines::lighting) = PipelineRegistry::Get().Register<Vertex2D>({
		shaders::lighting_vs,
		shaders::lighting_ps,
		std::nullopt,
		root_signatures::lighting,
		Format::UNKNOWN,
		{ Format::R8G8B8A8_UNORM },
		1,
		PipelineType::GRAPHICS_PIPELINE,
		CullMode::CULL_BACK,
		false,
		true,
		TopologyType::TRIANGLE
	});

} /* wr */
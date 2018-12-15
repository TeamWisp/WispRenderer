#pragma once

#include "engine_registry.hpp"

#include "pipeline_registry.hpp"
#include "root_signature_registry.hpp"
#include "shader_registry.hpp"
#include "rt_pipeline_registry.hpp"
#include "d3d12/d3d12_structs.hpp"

#define REGISTER(type, registry) decltype(type) type = registry::Get().Register

#define DESC_RANGE_ARRAY(name, ...) std::vector<CD3DX12_DESCRIPTOR_RANGE> name { __VA_ARGS__ };
#define DESC_RANGE(...) [] { CD3DX12_DESCRIPTOR_RANGE r; r.Init(__VA_ARGS__); return r; }()

#define ROOT_PARAM(func) [] { CD3DX12_ROOT_PARAMETER d; d.func; return d; }()
#define SAMPLER(...) { __VA_ARGS__ }

namespace wr
{
	DESC_RANGE_ARRAY(ranges_basic,
		DESC_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0)
	);
	REGISTER(root_signatures::basic, RootSignatureRegistry)({
		RootSignatureDescription::Parameters({
			ROOT_PARAM(InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX)),
			ROOT_PARAM(InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX)),
			ROOT_PARAM(InitAsDescriptorTable(ranges_basic.size(), ranges_basic.data(), D3D12_SHADER_VISIBILITY_PIXEL)),
		}),
		RootSignatureDescription::Samplers({
			{ TextureFilter::FILTER_LINEAR, TextureAddressMode::TAM_CLAMP }
		})
	});

	DESC_RANGE_ARRAY(srv_ranges,
		DESC_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0)
	);
	DESC_RANGE_ARRAY(uav_ranges,
		DESC_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0)
	);
	REGISTER(root_signatures::deferred_composition, RootSignatureRegistry)({
		RootSignatureDescription::Parameters({
			ROOT_PARAM(InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL)),
			ROOT_PARAM(InitAsDescriptorTable(srv_ranges.size(), srv_ranges.data(), D3D12_SHADER_VISIBILITY_ALL)),
			ROOT_PARAM(InitAsDescriptorTable(uav_ranges.size(), uav_ranges.data(), D3D12_SHADER_VISIBILITY_ALL)),
		}),
		RootSignatureDescription::Samplers({
			{ TextureFilter::FILTER_POINT, TextureAddressMode::TAM_BORDER }
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

	REGISTER(pipelines::basic_deferred, PipelineRegistry)<Vertex>({
		PipelineDescription::VertexShader(shaders::basic_vs),
		PipelineDescription::PixelShader(shaders::basic_ps),
		PipelineDescription::ComputeShader(std::nullopt),
		PipelineDescription::RootSignature(root_signatures::basic),
		PipelineDescription::DSVFormat(Format::D32_FLOAT),
		PipelineDescription::RTVFormats(std::array<Format, 8>{
			Format::R32G32B32A32_FLOAT,
			Format::R32G32B32A32_FLOAT
		}),
		PipelineDescription::NumRTVFormats(3),
		PipelineDescription::Type(PipelineType::GRAPHICS_PIPELINE),
		PipelineDescription::CullMode(CullMode::CULL_BACK),
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
		PipelineDescription::RTVFormats(std::array<Format, 8>{
			Format::R8G8B8A8_UNORM
		}),
		PipelineDescription::NumRTVFormats(1),
		PipelineDescription::Type(PipelineType::COMPUTE_PIPELINE),
		PipelineDescription::CullMode(CullMode::CULL_BACK),
		PipelineDescription::Depth(false),
		PipelineDescription::CounterClockwise(true),
		PipelineDescription::TopologyType(TopologyType::TRIANGLE)
	});

	/* ### Raytracing ### */
	REGISTER(shaders::rt_lib, ShaderRegistry)({
		ShaderDescription::Path("resources/shaders/raytracing.hlsl"),
		ShaderDescription::Entry("RaygenEntry"),
		ShaderDescription::Type(ShaderType::LIBRARY_SHADER)
	});

	D3D12_DESCRIPTOR_RANGE r[1] = {
		//{ D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, 0 },
		{ D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, 0 }
	};

	REGISTER(root_signatures::rt_test_global, RootSignatureRegistry)({
		RootSignatureDescription::Parameters({
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsDescriptorTable(1, r); return d; }(),
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsShaderResourceView(0); return d; }(),
			[] { CD3DX12_ROOT_PARAMETER d; d.InitAsConstantBufferView(0); return d; }(),
		}),
		RootSignatureDescription::Samplers({
			// No Samplers
		}),
		RootSignatureDescription::DXR(true)
	});

	REGISTER(root_signatures::rt_test_local, RootSignatureRegistry)({
		RootSignatureDescription::Parameters({
			// No parameters
		}),
		RootSignatureDescription::Samplers({
			// No samplers
		}),
		RootSignatureDescription::DXR(true),
		RootSignatureDescription::DXRLocal(true)
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
	
	REGISTER(state_objects::state_object, RTPipelineRegistry)(
	{
		StateObjectDescription::StateObjectDesc(so_desc.first),
		StateObjectDescription::LibraryDesc(so_desc.second),
		StateObjectDescription::MaxPayloadSize(sizeof(float) * 4),
		StateObjectDescription::MaxAttributeSize(sizeof(float) * 2),
		StateObjectDescription::MaxRecursionDepth(1),
		StateObjectDescription::GlobalRootSignature(root_signatures::rt_test_global),
		StateObjectDescription::LocalRootSignatures(std::vector<RegistryHandle>{ root_signatures::rt_test_local }),
	});

} /* wr */
// Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "d3d12_functions.hpp"

#include <variant>

#include "d3d12_defines.hpp"

namespace wr::d3d12
{

	namespace internal
	{

		[[nodiscard]] D3D12_GRAPHICS_PIPELINE_STATE_DESC GetGraphicsPipelineStateDescriptor(desc::PipelineStateDesc descriptor,
			std::vector<D3D12_INPUT_ELEMENT_DESC> const & input_layout,
			RootSignature* root_signature,
			Shader* vertex_shader,
			Shader* pixel_shader)
		{
			D3D12_BLEND_DESC blend_desc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

			D3D12_DEPTH_STENCIL_DESC depth_stencil_state = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
			if (descriptor.m_dsv_format == Format::UNKNOWN)
			{
				depth_stencil_state.DepthEnable = false;
				depth_stencil_state.StencilEnable = false;
			}

			D3D12_RASTERIZER_DESC rasterize_desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			rasterize_desc.FrontCounterClockwise = descriptor.m_counter_clockwise;
			depth_stencil_state.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
			rasterize_desc.CullMode = (D3D12_CULL_MODE)descriptor.m_cull_mode;

			DXGI_SAMPLE_DESC sample_desc = { 1, 0 };

			D3D12_INPUT_LAYOUT_DESC input_layout_desc = {};
			input_layout_desc.NumElements = static_cast<std::uint32_t>(input_layout.size());
			input_layout_desc.pInputElementDescs = input_layout.data();

			D3D12_SHADER_BYTECODE vs_bytecode = {};
			vs_bytecode.BytecodeLength = vertex_shader->m_native->GetBufferSize();
			vs_bytecode.pShaderBytecode = vertex_shader->m_native->GetBufferPointer();

			D3D12_SHADER_BYTECODE ps_bytecode = {};
			ps_bytecode.BytecodeLength = pixel_shader->m_native->GetBufferSize();
			ps_bytecode.pShaderBytecode = pixel_shader->m_native->GetBufferPointer();

			D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
			pso_desc.PrimitiveTopologyType = (D3D12_PRIMITIVE_TOPOLOGY_TYPE)descriptor.m_topology_type;
			for (auto i = 0u; i < descriptor.m_num_rtv_formats; i++)
			{ // FIXME: memcpy
				pso_desc.RTVFormats[i] = (DXGI_FORMAT)descriptor.m_rtv_formats[i];
			}
			pso_desc.DSVFormat = (DXGI_FORMAT)descriptor.m_dsv_format;
			pso_desc.SampleDesc = sample_desc;
			pso_desc.SampleMask = 0xffffffff;
			pso_desc.RasterizerState = rasterize_desc;
			pso_desc.BlendState = blend_desc;
			pso_desc.DepthStencilState = depth_stencil_state;
			pso_desc.NumRenderTargets = descriptor.m_num_rtv_formats;
			pso_desc.pRootSignature = root_signature->m_native;
			pso_desc.VS = vs_bytecode;
			pso_desc.PS = ps_bytecode;
			pso_desc.InputLayout = input_layout_desc;

			return pso_desc;
		}

		[[nodiscard]] D3D12_COMPUTE_PIPELINE_STATE_DESC GetComputePipelineStateDescriptor(RootSignature* root_signature, Shader* compute_shader)
		{
			D3D12_SHADER_BYTECODE cs_bytecode = {};
			cs_bytecode.BytecodeLength = compute_shader->m_native->GetBufferSize();
			cs_bytecode.pShaderBytecode = compute_shader->m_native->GetBufferPointer();

			D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc = {};
			pso_desc.pRootSignature = root_signature->m_native;
			pso_desc.CS = cs_bytecode;

			return pso_desc;
		}

	} /* internal */

	PipelineState* CreatePipelineState()
	{
		return new PipelineState();
	}

	void SetName(PipelineState * pipeline_state, std::wstring name)
	{
		pipeline_state->m_native->SetName(name.c_str());
	}

	void SetVertexShader(PipelineState* pipeline_state, Shader* shader)
	{
		pipeline_state->m_vertex_shader = shader;
	}

	void SetFragmentShader(PipelineState* pipeline_state, Shader* shader)
	{
		pipeline_state->m_pixel_shader = shader;
	}

	void SetComputeShader(PipelineState* pipeline_state, Shader* shader)
	{
		pipeline_state->m_compute_shader = shader;
	}

	void SetRootSignature(PipelineState* pipeline_state, RootSignature* root_signature)
	{
		pipeline_state->m_root_signature = root_signature;
	}

	void FinalizePipeline(PipelineState* pipeline_state, Device* device, desc::PipelineStateDesc desc)
	{
		auto n_device = device->m_native;

		pipeline_state->m_device = device;
		pipeline_state->m_desc = desc;

		std::variant<D3D12_GRAPHICS_PIPELINE_STATE_DESC, D3D12_COMPUTE_PIPELINE_STATE_DESC> pso_desc;
		switch (desc.m_type)
		{
		case PipelineType::GRAPHICS_PIPELINE:
		{
			pso_desc = internal::GetGraphicsPipelineStateDescriptor(desc,
				desc.m_input_layout,
				pipeline_state->m_root_signature,
				pipeline_state->m_vertex_shader,
				pipeline_state->m_pixel_shader);
		}
		break;
		case PipelineType::COMPUTE_PIPELINE:
		{
			pso_desc = internal::GetComputePipelineStateDescriptor(pipeline_state->m_root_signature,
				pipeline_state->m_compute_shader);
		}
		break;
		}

		HRESULT hr = E_FAIL;
		if (std::holds_alternative<D3D12_GRAPHICS_PIPELINE_STATE_DESC>(pso_desc))
		{
			hr = n_device->CreateGraphicsPipelineState(&std::get<D3D12_GRAPHICS_PIPELINE_STATE_DESC>(pso_desc), IID_PPV_ARGS(&pipeline_state->m_native));
		}
		else if (std::holds_alternative<D3D12_COMPUTE_PIPELINE_STATE_DESC>(pso_desc))
		{
			hr = n_device->CreateComputePipelineState(&std::get<D3D12_COMPUTE_PIPELINE_STATE_DESC>(pso_desc), IID_PPV_ARGS(&pipeline_state->m_native));
		}
		else
		{
			LOGC("Variant seems to be empty...");
		}
		if (FAILED(hr))
		{
			LOGC("Failed to create graphics pipeline");
		}
		NAME_D3D12RESOURCE(pipeline_state->m_native)
	}

	void RefinalizePipeline(PipelineState* pipeline_state)
	{
		FinalizePipeline(pipeline_state, pipeline_state->m_device, pipeline_state->m_desc);
	}

	void Destroy(PipelineState* pipeline_state)
	{
		SAFE_RELEASE(pipeline_state->m_native);
		delete pipeline_state;
	}

} /* wr::d3d12 */
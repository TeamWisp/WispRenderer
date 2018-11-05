#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <array>

#include "d3d12_enums.hpp"
#include "d3dx12.hpp"

namespace d3d12
{

	namespace desc
	{
		
		struct RenderTargetDesc
		{
			bool m_create_dsv_buffer = true;
			Format m_dsv_format;
			std::array<Format, 8> m_rtv_formats;
			unsigned int m_num_rtv_formats;
		};

		struct PipelineStateDesc
		{
			Format m_dsv_format;
			std::array<Format, 8> m_rtv_formats;
			unsigned int m_num_rtv_formats;

			PipelineType m_type = PipelineType::GRAPHICS_PIPELINE;
			CullMode m_cull_mode = CullMode::CULL_BACK;
			bool m_depth_eenabled = false;
			bool m_counter_clockwise = true;
			TopologyType m_topology_type = TopologyType::TRIANGLE;

			std::vector<D3D12_INPUT_ELEMENT_DESC> m_input_layout;
		};

		struct SamplerDesc
		{
			TextureFilter m_filter;
			TextureAddressMode m_address_mode;
			BorderColor m_border_color = BorderColor::BORDER_WHITE;
		};

		struct RootSignatureDesc
		{
			std::vector<CD3DX12_ROOT_PARAMETER> m_parameters;
			std::vector<desc::SamplerDesc> m_samplers;
		};

		struct DescriptorHeapDesc
		{
			unsigned int m_num_descriptors;
			DescriptorHeapType m_type;
			bool m_shader_visible = true;
		};

	} /* desc */

	struct Device
	{
		IDXGIAdapter4* m_adapter;
		ID3D12Device4* m_native;
		IDXGIFactory6* m_dxgi_factory;
		D3D_FEATURE_LEVEL m_feature_level;

		SYSTEM_INFO m_sys_info;
		DXGI_ADAPTER_DESC3 m_adapter_info;
		ID3D12Debug3* m_debug_controller;
		ID3D12InfoQueue* m_info_queue;
	};

	struct CommandQueue
	{
		ID3D12CommandQueue* m_native;
	};

	template<int N>
	struct CommandList
	{
		std::array<ID3D12CommandAllocator*, N> m_allocators;
		ID3D12GraphicsCommandList4* m_native;
	};

	using VCommandList = CommandList<3>;

	template<int N>
	struct RenderTarget
	{
		desc::RenderTargetDesc m_create_info;
		unsigned int m_frame_idx;

		std::array<ID3D12Resource*, N> m_render_targets;
		ID3D12DescriptorHeap* m_rtv_descriptor_heap;
		unsigned int m_rtv_descriptor_increment_size;

		ID3D12Resource* m_depth_stencil_buffer;
		ID3D12DescriptorHeap* m_depth_stencil_resource_heap;
	};

	template <int N = 3>
	struct RenderWindow : public RenderTarget<N>
	{
		IDXGISwapChain4* m_swap_chain;
	};

	template<int N>
	struct Fence
	{
		std::array<ID3D12Fence1*, N> m_native;
		HANDLE m_fence_event;
		std::array<UINT64, N> m_fence_value;
	};

	using VFence = Fence<3>;

	struct RootSignature
	{
		desc::RootSignatureDesc m_create_info;
		ID3D12RootSignature* m_native;
	};

	struct Shader
	{
		ID3DBlob* m_native;
#ifdef _DEBUG
		std::string m_path;
		ShaderType m_type;
#endif
	};

	struct PipelineState
	{
#ifdef _DEBUG
		desc::PipelineStateDesc m_desc;
#endif
		PipelineType m_type;
		ID3D12PipelineState* m_native;
		RootSignature* m_root_signature;
		Shader* m_vertex_shader;
		Shader* m_pixel_shader;
		Shader* m_compute_shader;
		D3D12_PRIMITIVE_TOPOLOGY_TYPE m_topology_type;
		std::vector<D3D12_INPUT_ELEMENT_DESC> m_input_layout;
	};

	struct Viewport
	{
		D3D12_VIEWPORT m_viewport;
		D3D12_RECT m_scissor_rect;
	};

	struct DescriptorHeap
	{
		desc::DescriptorHeapDesc m_create_info;
		ID3D12DescriptorHeap* m_native;
		unsigned int m_increment_size;
	};

	struct DescHeapCPUHandle
	{
		D3D12_GPU_DESCRIPTOR_HANDLE m_native;
	};

	struct DescHeapGPUHandle
	{
		D3D12_CPU_DESCRIPTOR_HANDLE m_native;
	};

} /* d3d12 */

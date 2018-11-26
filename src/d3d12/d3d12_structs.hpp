#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <optional>
#include <array>

#include "d3d12_enums.hpp"
#include "d3d12_settings.hpp"
#include "d3dx12.hpp"

namespace wr::d3d12
{

	namespace desc
	{
		
		struct RenderTargetDesc
		{
			ResourceState m_initial_state = ResourceState::RENDER_TARGET;
			bool m_create_dsv_buffer = true;
			Format m_dsv_format;
			std::array<Format, 8> m_rtv_formats;
			unsigned int m_num_rtv_formats;
			float m_clear_color[4] = { 0, 0, 0, 1 };
		};

		struct PipelineStateDesc
		{
			Format m_dsv_format;
			std::array<Format, 8> m_rtv_formats;
			unsigned int m_num_rtv_formats;

			PipelineType m_type = PipelineType::GRAPHICS_PIPELINE;
			CullMode m_cull_mode = CullMode::CULL_BACK;
			bool m_depth_enabled = false;
			bool m_counter_clockwise = false;
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
		ID3D12Debug1* m_debug_controller;
		ID3D12InfoQueue* m_info_queue;
	};

	struct CommandQueue
	{
		ID3D12CommandQueue* m_native;
	};

	struct CommandList
	{
		std::vector<ID3D12CommandAllocator*> m_allocators;
		ID3D12GraphicsCommandList3* m_native;
	};

	struct RenderTarget
	{
		desc::RenderTargetDesc m_create_info;
		unsigned int m_frame_idx;
		unsigned int m_num_render_targets;

		std::vector<ID3D12Resource*> m_render_targets;
		ID3D12DescriptorHeap* m_rtv_descriptor_heap;
		unsigned int m_rtv_descriptor_increment_size;

		ID3D12Resource* m_depth_stencil_buffer;
		ID3D12DescriptorHeap* m_depth_stencil_resource_heap;
	};

	struct RenderWindow : public RenderTarget
	{
		IDXGISwapChain4* m_swap_chain;
	};

	struct Fence
	{
		ID3D12Fence1* m_native;
		HANDLE m_fence_event;
		UINT64 m_fence_value;
	};

	struct RootSignature
	{
		desc::RootSignatureDesc m_create_info;
		ID3D12RootSignature* m_native;
	};

	struct Shader
	{
		ID3DBlob* m_native;
		std::string m_path;
		std::string m_entry;
		ShaderType m_type;
	};

	struct PipelineState
	{
		PipelineType m_type;
		ID3D12PipelineState* m_native;
		RootSignature* m_root_signature;
		Shader* m_vertex_shader;
		Shader* m_pixel_shader;
		Shader* m_compute_shader;
		D3D12_PRIMITIVE_TOPOLOGY_TYPE m_topology_type;
		std::vector<D3D12_INPUT_ELEMENT_DESC> m_input_layout; // TODO: This should be defaulted.
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
		D3D12_CPU_DESCRIPTOR_HANDLE m_native;
	};

	struct DescHeapGPUHandle
	{
		D3D12_GPU_DESCRIPTOR_HANDLE m_native;
	};

	struct StagingBuffer
	{
		ID3D12Resource* m_buffer;
		ID3D12Resource* m_staging;
		unsigned int m_size;
		unsigned int m_stride_in_bytes;
		void* m_data;
		ResourceState m_target_resource_state;
		D3D12_GPU_VIRTUAL_ADDRESS m_gpu_address;
		std::uint8_t* m_cpu_address;
		bool m_is_staged;
	};

	struct HeapResource;
	namespace detail
	{

		/*! Fallback detail structure */
		template<HeapOptimization O>
		struct Heap
		{
			
		};

		/*! Small Buffer Optimization */
		// TODO: Which one should we version? The heap or the resource? test perf!
		template<>
		struct Heap<HeapOptimization::SMALL_BUFFERS>
		{
			std::vector<HeapResource*> m_resources;
			std::uint8_t* m_cpu_address;
			ID3D12Resource* m_native;
		};

		/*! Big Buffer Optimization */
		// TODO: Which one should we version? The heap or the resource? test perf!
		template<>
		struct Heap<HeapOptimization::BIG_BUFFERS>
		{
			std::vector<std::pair<HeapResource*, std::vector<ID3D12Resource*>>> m_resources;
			ID3D12Heap* m_native;
		};

		template<>
		struct Heap<HeapOptimization::SMALL_STATIC_BUFFERS>
		{
			std::vector<HeapResource*> m_resources;
			D3D12_GPU_VIRTUAL_ADDRESS m_gpu_address;
			ID3D12Resource* m_native;
		};

		template<>
		struct Heap<HeapOptimization::BIG_STATIC_BUFFERS> 
		{
			std::vector<std::pair<HeapResource*, std::vector<ID3D12Resource*>>> m_resources;
			ID3D12Heap* m_native;
		};

	} /* detail */

	template<HeapOptimization O>
	struct Heap : detail::Heap<O>
	{
		bool m_mapped;
		unsigned int m_versioning_count;
		std::uint64_t m_current_offset;
		std::uint64_t m_heap_size;
		std::uint64_t m_alignment;
		std::vector<std::uint64_t> m_bitmap;
	};

	struct HeapResource
	{
		union 
		{
			Heap<HeapOptimization::SMALL_BUFFERS>* m_heap_sbo;
			Heap<HeapOptimization::SMALL_STATIC_BUFFERS>* m_heap_ssbo;
			Heap<HeapOptimization::BIG_BUFFERS>* m_heap_bbo;
			Heap<HeapOptimization::BIG_STATIC_BUFFERS>* m_heap_bsbo;
		};

		std::vector<D3D12_GPU_VIRTUAL_ADDRESS> m_gpu_addresses;
		std::optional<std::vector<std::uint8_t*>> m_cpu_addresses;
		std::uint64_t m_unaligned_size;
		std::uint64_t m_begin_offset;
		std::size_t m_heap_vector_location;
		std::size_t m_stride;
		HeapOptimization m_resource_heap_optimization;
	};

} /* wr::d3d12 */
#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <optional>
#include <utility>
#include <dxcapi.h>
#include <array>
#include <bitset>
#include <D3D12RaytracingFallback.h>
#include <DirectXMath.h>

#include "../structs.hpp"

#include "d3d12_enums.hpp"
#include "d3d12_settings.hpp"
#include "d3dx12.hpp"

namespace wr
{
	class DynamicDescriptorHeap;
	class DescriptorAllocation;
	class RTDescriptorHeap;
}

namespace wr::d3d12
{

	// Forward declare
	struct StagingBuffer;
	struct Shader;
	struct RootSignature;

	namespace desc
	{

		struct RenderTargetDesc
		{
			ResourceState m_initial_state = ResourceState::RENDER_TARGET;
			bool m_create_dsv_buffer = true;
			Format m_dsv_format;
			std::array<Format, 8> m_rtv_formats;
			unsigned int m_num_rtv_formats;
			float m_clear_color[4] = { 0.f, 0.f, 0.f, 0.f };
		};

		struct TextureDesc
		{
			ResourceState m_initial_state = ResourceState::COPY_DEST;
			Format m_texture_format = Format::UNKNOWN;

			unsigned int m_width = 0;
			unsigned int m_height = 0;;
			unsigned int m_depth = 0;;
			unsigned int m_array_size = 0;;
			unsigned int m_mip_levels = 0;;

			bool m_is_cubemap = false;
		};

		struct ReadbackDesc
		{
			unsigned int m_buffer_width;
			unsigned int m_buffer_height;
			unsigned int m_bytes_per_pixel;
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
			bool m_rtx = false;
			bool m_rt_local = false;
		};

		struct DescriptorHeapDesc
		{
			unsigned int m_num_descriptors = 1;
			DescriptorHeapType m_type = DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV;
			bool m_shader_visible = true;
			uint32_t m_versions = 1;
		};

		struct GeometryDesc
		{
			StagingBuffer* vertex_buffer = nullptr;
			std::optional<StagingBuffer*> index_buffer;

			std::uint32_t m_num_vertices = 0;
			std::uint32_t m_num_indices = 0;
			std::uint32_t m_vertices_offset = 0;
			std::uint32_t m_indices_offset = 0;

			std::uint32_t m_vertex_stride = 0;
		};

		struct HitGroupDesc
		{
			std::wstring m_hitgroup;
			std::wstring m_closesthit;
			std::optional<std::wstring> m_anyhit;
		};

		struct StateObjectDesc
		{
			Shader* m_library;
			std::vector<std::wstring> m_library_exports;
			std::vector<HitGroupDesc> m_hit_groups; // first = hit group | second = entry

			std::uint32_t max_payload_size;
			std::uint32_t max_attributes_size;
			std::uint32_t max_recursion_depth;

			std::optional<RootSignature*> global_root_signature;
			std::optional<std::vector<RootSignature*>> local_root_signatures;
		};

	} /* desc */

	struct Device
	{
		IDXGIAdapter4* m_adapter;
		ID3D12Device5* m_native;
		IDXGIFactory6* m_dxgi_factory;
		D3D_FEATURE_LEVEL m_feature_level;

		SYSTEM_INFO m_sys_info;
		DXGI_ADAPTER_DESC3 m_adapter_info;
		ID3D12Debug1* m_debug_controller;
		ID3D12InfoQueue* m_info_queue;
		
		static IDxcCompiler2* m_compiler;

		// Optional supported formats
		std::bitset<DXGI_FORMAT_V408> m_optional_formats;

		// Fallback
		bool m_dxr_fallback_support;
		bool m_dxr_support;
		RaytracingType m_rt_type;
		ID3D12RaytracingFallbackDevice* m_fallback_native;

		bool IsFallback() 
		{
			return m_rt_type == RaytracingType::FALLBACK;
		}
	};

	struct CommandQueue
	{
		ID3D12CommandQueue* m_native;
	};

	struct CommandList
	{
		std::vector<ID3D12CommandAllocator*> m_allocators;
		ID3D12GraphicsCommandList4* m_native;
		ID3D12RaytracingFallbackCommandList* m_native_fallback;

		// Dynamic descriptor heap where staging happens
		std::unique_ptr<DynamicDescriptorHeap> m_dynamic_descriptor_heaps[static_cast<size_t>(DescriptorHeapType::DESC_HEAP_TYPE_NUM_TYPES)];
		
		// Heap used for RT, shared between all the ray tracing command lists
		std::shared_ptr<RTDescriptorHeap> m_rt_descriptor_heap;

		// Keep track of currently bound heaps, so that we can avoid changing when not needed
		ID3D12DescriptorHeap* m_descriptor_heaps[static_cast<size_t>(DescriptorHeapType::DESC_HEAP_TYPE_NUM_TYPES)];
	};

	struct CommandSignature
	{
		ID3D12CommandSignature* m_native;
	};

	struct RenderTarget
	{
		desc::RenderTargetDesc m_create_info;
		unsigned int m_frame_idx;
		unsigned int m_num_render_targets;
		unsigned int m_width;
		unsigned int m_height;

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

		std::uint32_t m_num_descriptors_per_table[32];
		std::uint32_t m_sampler_table_bit_mask;
		std::uint32_t m_descriptor_table_bit_mask;
	};

	struct PipelineState;
	struct StateObject;
	struct Shader
	{
		IDxcBlob* m_native;
		std::string m_path;
		std::string m_entry;
		ShaderType m_type;
	};

	struct PipelineState
	{
		ID3D12PipelineState* m_native;
		RootSignature* m_root_signature;
		Shader* m_vertex_shader;
		Shader* m_pixel_shader;
		Shader* m_compute_shader;
		Device* m_device; // only used for refinalization.
		desc::PipelineStateDesc m_desc;
	};

	struct Viewport
	{
		D3D12_VIEWPORT m_viewport;
		D3D12_RECT m_scissor_rect;
	};

	struct DescriptorHeap
	{
		desc::DescriptorHeapDesc m_create_info;
		std::vector<ID3D12DescriptorHeap*> m_native;
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
		ResourceState m_target_resource_state;
		D3D12_GPU_VIRTUAL_ADDRESS m_gpu_address;
		std::uint8_t* m_cpu_address;
		bool m_is_staged;
	};

	struct ReadbackBufferResource : ReadbackBuffer
	{
		ID3D12Resource* m_resource;
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
			ID3D12Resource* m_staging_buffer;
			std::uint8_t* m_cpu_address;
		};

		template<>
		struct Heap<HeapOptimization::BIG_STATIC_BUFFERS> 
		{
			std::vector<std::pair<HeapResource*, std::vector<ID3D12Resource*>>> m_resources;
			ID3D12Heap* m_native;
			ID3D12Resource* m_staging_buffer;
			std::uint8_t* m_cpu_address;
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
		bool m_used_as_uav;
		HeapOptimization m_resource_heap_optimization;
		std::vector<ResourceState> m_states;
	};

	struct IndirectCommandBuffer
	{
		std::size_t m_num_commands;
		std::size_t m_num_max_commands;
		std::size_t m_command_size;
		std::vector<ID3D12Resource*> m_native;
		std::vector<ID3D12Resource*> m_native_upload;
	};

	struct StateObject
	{
		ID3D12StateObject* m_native;
		RootSignature* m_global_root_signature;
		ID3D12StateObjectProperties* m_properties;

		ID3D12RaytracingFallbackStateObject* m_fallback_native;

		desc::StateObjectDesc m_desc;
		Device* m_device; // Only for refinalization.
	};

	struct AccelerationStructure
	{
		ID3D12Resource* m_scratch;														 // Scratch memory for AS builder
		std::array<ID3D12Resource*, d3d12::settings::num_back_buffers> m_natives;        // Where the AS is
		std::array<ID3D12Resource*, d3d12::settings::num_back_buffers> m_instance_descs; // Hold the matrices of the instances
		WRAPPED_GPU_POINTER m_fallback_tlas_ptr;
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO m_prebuild_info;
	};

	namespace desc
	{
		struct BlasDesc
		{
			d3d12::AccelerationStructure m_as;
			std::uint64_t m_material;
			DirectX::XMMATRIX m_transform;
		};
	} /* desc */

	struct ShaderRecord
	{
		std::pair<void*, std::uint64_t> m_shader_identifier;
		std::pair<void*, std::uint64_t> m_local_root_args;
	};

	struct ShaderTable
	{
		std::uint8_t* m_mapped_shader_records;
		std::uint64_t m_shader_record_size;
		std::vector<ShaderRecord> m_shader_records;
		ID3D12Resource* m_resource;
	};

} /* wr::d3d12 */

/*!
 * Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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
			Format m_dsv_format = wr::Format::UNKNOWN;
			std::array<Format, 8> m_rtv_formats = { wr::Format::UNKNOWN };
			std::uint32_t m_num_rtv_formats = 0u;
			float m_clear_color[4] = { 0.f, 0.f, 0.f, 0.f };
		};

		struct TextureDesc
		{
			ResourceState m_initial_state = ResourceState::COPY_DEST;
			Format m_texture_format = Format::UNKNOWN;

			std::uint32_t m_width = 0u;
			std::uint32_t m_height = 0u;
			std::uint32_t m_depth = 0u;
			std::uint32_t m_array_size = 0u;
			std::uint32_t m_mip_levels = 0u;

			bool m_is_cubemap = false;
		};

		struct PipelineStateDesc
		{
			Format m_dsv_format;
			std::array<Format, 8> m_rtv_formats = { wr::Format::UNKNOWN };
			std::uint32_t m_num_rtv_formats = 0U;

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
			std::uint32_t m_num_descriptors = 1u;
			DescriptorHeapType m_type = DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV;
			bool m_shader_visible = true;
			uint32_t m_versions = 1u;
		};

		struct GeometryDesc
		{
			StagingBuffer* vertex_buffer = nullptr;
			std::optional<StagingBuffer*> index_buffer = std::nullopt;

			std::uint32_t m_num_vertices = 0u;
			std::uint32_t m_num_indices = 0u;
			std::uint32_t m_vertices_offset = 0u;
			std::uint32_t m_indices_offset = 0u;

			std::uint32_t m_vertex_stride = 0u;
		};

		struct HitGroupDesc
		{
			std::wstring m_hitgroup;
			std::wstring m_closesthit;
			std::optional<std::wstring> m_anyhit;
		};

		struct StateObjectDesc
		{
			Shader* m_library = nullptr;
			std::vector<std::wstring> m_library_exports;
			std::vector<HitGroupDesc> m_hit_groups; // hitgroup, closesthit and anyhit

			std::uint32_t max_payload_size = 0u;
			std::uint32_t max_attributes_size = 0u;
			std::uint32_t max_recursion_depth = 0u;

			std::optional<RootSignature*> global_root_signature = std::nullopt;
			std::optional<std::vector<RootSignature*>> local_root_signatures = { std::nullopt };
		};

	} /* desc */

	struct Device
	{
		IDXGIAdapter4* m_adapter = nullptr;
		ID3D12Device5* m_native = nullptr;
		IDXGIFactory6* m_dxgi_factory = nullptr;
		D3D_FEATURE_LEVEL m_feature_level;

		SYSTEM_INFO m_sys_info;
		DXGI_ADAPTER_DESC3 m_adapter_info;
		ID3D12Debug1* m_debug_controller = nullptr;
		ID3D12InfoQueue* m_info_queue = nullptr;
		
		static IDxcCompiler2* m_compiler;

		// Optional supported formats
		std::bitset<DXGI_FORMAT_V408> m_optional_formats;

		// Fallback
		bool m_dxr_fallback_support = false;
		bool m_dxr_support = false;
		RaytracingType m_rt_type;
		ID3D12RaytracingFallbackDevice* m_fallback_native = nullptr;

		bool IsFallback() 
		{
			return m_rt_type == RaytracingType::FALLBACK;
		}
	};

	struct CommandQueue
	{
		ID3D12CommandQueue* m_native = nullptr;
	};

	struct CommandList
	{
		std::vector<ID3D12CommandAllocator*> m_allocators;
		ID3D12GraphicsCommandList4* m_native = nullptr;
		ID3D12RaytracingFallbackCommandList* m_native_fallback = nullptr;

		// Dynamic descriptor heap where staging happens
		std::unique_ptr<DynamicDescriptorHeap> m_dynamic_descriptor_heaps[static_cast<size_t>(DescriptorHeapType::DESC_HEAP_TYPE_NUM_TYPES)];
		
		// Heap used for RT, shared between all the ray tracing command lists
		std::shared_ptr<RTDescriptorHeap> m_rt_descriptor_heap;

		// Keep track of currently bound heaps, so that we can avoid changing when not needed
		ID3D12DescriptorHeap* m_descriptor_heaps[static_cast<size_t>(DescriptorHeapType::DESC_HEAP_TYPE_NUM_TYPES)];
	};

	struct CommandSignature
	{
		ID3D12CommandSignature* m_native = nullptr;
	};

	struct RenderTarget
	{
		desc::RenderTargetDesc m_create_info;
		std::uint32_t m_frame_idx = 0u;
		std::uint32_t m_num_render_targets = 0u;
		std::uint32_t m_width = 0u;
		std::uint32_t m_height = 0u;

		std::vector<ID3D12Resource*> m_render_targets;
		ID3D12DescriptorHeap* m_rtv_descriptor_heap = nullptr;
		std::uint32_t m_rtv_descriptor_increment_size = 0u;

		ID3D12Resource* m_depth_stencil_buffer = nullptr;
		ID3D12DescriptorHeap* m_depth_stencil_resource_heap = nullptr;
	};

	struct RenderWindow : public RenderTarget
	{
		IDXGISwapChain4* m_swap_chain = nullptr;
	};

	struct Fence
	{
		ID3D12Fence1* m_native = nullptr;
		HANDLE m_fence_event = nullptr;
		UINT64 m_fence_value = static_cast<UINT64>(0u);
	};

	struct RootSignature
	{
		desc::RootSignatureDesc m_create_info;
		ID3D12RootSignature* m_native = nullptr;

		std::uint32_t m_num_descriptors_per_table[32] = { 0u };
		std::uint32_t m_sampler_table_bit_mask = 0u;
		std::uint32_t m_descriptor_table_bit_mask = 0u;
	};

	struct PipelineState;
	struct StateObject;
	struct Shader
	{
		IDxcBlob* m_native = nullptr;
		std::string m_path = "";
		std::string m_entry = "";
		ShaderType m_type;
		std::vector<std::pair<std::wstring,std::wstring>> m_defines;
	};

	struct PipelineState
	{
		ID3D12PipelineState* m_native = nullptr;
		RootSignature* m_root_signature = nullptr;
		Shader* m_vertex_shader = nullptr;
		Shader* m_pixel_shader = nullptr;
		Shader* m_compute_shader = nullptr;
		Device* m_device = nullptr; // only used for refinalization.
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
		std::uint32_t m_increment_size = 0u;
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
		ID3D12Resource* m_buffer = nullptr;
		ID3D12Resource* m_staging = nullptr;
		std::uint64_t m_size = 0u;
		std::uint64_t m_stride_in_bytes = 0u;
		ResourceState m_target_resource_state;
		D3D12_GPU_VIRTUAL_ADDRESS m_gpu_address;
		std::uint8_t* m_cpu_address = nullptr;
		bool m_is_staged = false;
	};

	struct ReadbackBufferResource : ReadbackBuffer
	{
		ID3D12Resource* m_resource = nullptr;
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
			std::uint8_t* m_cpu_address = nullptr;
			ID3D12Resource* m_native = nullptr;
		};

		/*! Big Buffer Optimization */
		// TODO: Which one should we version? The heap or the resource? test perf!
		template<>
		struct Heap<HeapOptimization::BIG_BUFFERS>
		{
			std::vector<std::pair<HeapResource*, std::vector<ID3D12Resource*>>> m_resources;
			ID3D12Heap* m_native = nullptr;
		};

		template<>
		struct Heap<HeapOptimization::SMALL_STATIC_BUFFERS>
		{
			std::vector<HeapResource*> m_resources;
			D3D12_GPU_VIRTUAL_ADDRESS m_gpu_address;
			ID3D12Resource* m_native = nullptr;
			ID3D12Resource* m_staging_buffer = nullptr;
			std::uint8_t* m_cpu_address = nullptr;
		};

		template<>
		struct Heap<HeapOptimization::BIG_STATIC_BUFFERS> 
		{
			std::vector<std::pair<HeapResource*, std::vector<ID3D12Resource*>>> m_resources;
			ID3D12Heap* m_native = nullptr;
			ID3D12Resource* m_staging_buffer = nullptr;
			std::uint8_t* m_cpu_address = nullptr;
		};

	} /* detail */

	template<HeapOptimization O>
	struct Heap : detail::Heap<O>
	{
		bool m_mapped;
		std::uint32_t m_versioning_count = 0u;
		std::uint64_t m_current_offset = 0u;
		std::uint64_t m_heap_size = 0u;
		std::uint64_t m_alignment = 0u;
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
		std::uint64_t m_unaligned_size = 0u;
		std::uint64_t m_begin_offset = 0u;
		std::size_t m_heap_vector_location = 0;
		std::size_t m_stride = 0;
		bool m_used_as_uav = false;
		HeapOptimization m_resource_heap_optimization;
		std::vector<ResourceState> m_states;
	};

	struct IndirectCommandBuffer
	{
		std::size_t m_num_commands = 0u;
		std::size_t m_num_max_commands = 0u;
		std::size_t m_command_size = 0u;
		std::vector<ID3D12Resource*> m_native;
		std::vector<ID3D12Resource*> m_native_upload;
	};

	struct StateObject
	{
		ID3D12StateObject* m_native = nullptr;
		RootSignature* m_global_root_signature = nullptr;
		ID3D12StateObjectProperties* m_properties = nullptr;

		ID3D12RaytracingFallbackStateObject* m_fallback_native = nullptr;

		desc::StateObjectDesc m_desc;
		Device* m_device = nullptr; // Only for refinalization.
	};

	struct AccelerationStructure
	{
		ID3D12Resource* m_scratch = nullptr;														 // Scratch memory for AS builder
		std::array<ID3D12Resource*, d3d12::settings::num_back_buffers> m_natives = { nullptr };        // Where the AS is
		std::array<ID3D12Resource*, d3d12::settings::num_back_buffers> m_instance_descs = { nullptr }; // Hold the matrices of the instances
		WRAPPED_GPU_POINTER m_fallback_tlas_ptr;
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO m_prebuild_info;
	};

	namespace desc
	{
		struct BlasDesc
		{
			d3d12::AccelerationStructure m_as;
			std::uint64_t m_material = 0u;
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
		std::uint8_t* m_mapped_shader_records = nullptr;
		std::uint64_t m_shader_record_size = 0u;
		std::vector<ShaderRecord> m_shader_records;
		ID3D12Resource* m_resource = nullptr;
	};

} /* wr::d3d12 */

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

#include "../frame_graph/frame_graph.hpp"
#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "../render_tasks/d3d12_deferred_main.hpp"
#include "../render_tasks/d3d12_post_processing.hpp"

#include "d3d12_raytracing_task.hpp"

namespace wr
{
	struct BloomExtractBrightData
	{
		d3d12::RenderTarget* out_source_rt = nullptr;
		d3d12::RenderTarget* out_source_emissive = nullptr;
		d3d12::RenderTarget* out_source_coc = nullptr;
		d3d12::PipelineState* out_pipeline = nullptr;
		ID3D12Resource* out_previous = nullptr;
		DescriptorAllocator* out_allocator = nullptr;
		DescriptorAllocation out_allocation;
	};

	namespace internal
	{
	
		template<typename T, typename T1>
		inline void SetupBloomExtractBrightTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<BloomExtractBrightData>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);

			if (!resize)
			{
				data.out_allocator = new DescriptorAllocator(n_render_system, wr::DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV);
				data.out_allocation = data.out_allocator->Allocate(4);
			}

			auto& ps_registry = PipelineRegistry::Get();
			data.out_pipeline = ((d3d12::PipelineState*)ps_registry.Find(pipelines::bloom_extract_bright));

			auto source_rt = data.out_source_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<T>());
			auto source_emissive = data.out_source_emissive = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<T1>());

			// Bright output for bloom
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::bloom_extract_bright, params::BloomExtractBrightE::OUTPUT_BRIGHT)));
				d3d12::CreateUAVFromSpecificRTV(n_render_target, cpu_handle, 0, n_render_target->m_create_info.m_rtv_formats[0]);
			}
			// Source
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::bloom_extract_bright, params::BloomExtractBrightE::SOURCE)));
				d3d12::CreateSRVFromSpecificRTV(source_rt, cpu_handle, 0, source_rt->m_create_info.m_rtv_formats[0]);
			}

			// Depth buffer
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::bloom_extract_bright, params::BloomExtractBrightE::G_DEPTH)));
				d3d12::CreateSRVFromDSV(source_emissive, cpu_handle);
			}

			// Source emissive
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::bloom_extract_bright, params::BloomExtractBrightE::G_EMISSIVE)));
				d3d12::CreateSRVFromSpecificRTV(source_emissive, cpu_handle, 2, source_emissive->m_create_info.m_rtv_formats[2]);
			}
		}

		template<typename T, typename T1>
		inline void ExecuteBloomExtractBrightTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& device = n_render_system.m_device;
			auto& data = fg.GetData<BloomExtractBrightData>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);
			auto frame_idx = n_render_system.GetFrameIdx();
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
			const auto viewport = n_render_system.m_viewport;

			auto source_rt = data.out_source_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<T>());
			auto source_emissive = data.out_source_emissive = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<T1>());

			// Bright output for bloom
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::bloom_extract_bright, params::BloomExtractBrightE::OUTPUT_BRIGHT)));
				d3d12::CreateUAVFromSpecificRTV(n_render_target, cpu_handle, 0, n_render_target->m_create_info.m_rtv_formats[0]);
			}
			// Source
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::bloom_extract_bright, params::BloomExtractBrightE::SOURCE)));
				d3d12::CreateSRVFromSpecificRTV(source_rt, cpu_handle, 0, source_rt->m_create_info.m_rtv_formats[0]);
			}
			// Depth buffer
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::bloom_extract_bright, params::BloomExtractBrightE::G_DEPTH)));
				d3d12::CreateSRVFromDSV(source_emissive, cpu_handle);
			}

			// Source emissive
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::bloom_extract_bright, params::BloomExtractBrightE::G_EMISSIVE)));
				d3d12::CreateSRVFromSpecificRTV(source_emissive, cpu_handle, 2, source_emissive->m_create_info.m_rtv_formats[2]);
			}



			d3d12::BindComputePipeline(cmd_list, data.out_pipeline);


			{
				constexpr unsigned int dest_b_idx = rs_layout::GetHeapLoc(params::bloom_extract_bright, params::BloomExtractBrightE::OUTPUT_BRIGHT);
				auto handle_uav = data.out_allocation.GetDescriptorHandle(dest_b_idx);
				d3d12::SetShaderUAV(cmd_list, 0, dest_b_idx, handle_uav);
			}

			{
				constexpr unsigned int source_idx = rs_layout::GetHeapLoc(params::bloom_extract_bright, params::BloomExtractBrightE::SOURCE);
				auto handle_b_srv = data.out_allocation.GetDescriptorHandle(source_idx);
				d3d12::SetShaderSRV(cmd_list, 0, source_idx, handle_b_srv);
			}

			{
				constexpr unsigned int source_idx = rs_layout::GetHeapLoc(params::bloom_extract_bright, params::BloomExtractBrightE::G_EMISSIVE);
				auto handle_b_srv = data.out_allocation.GetDescriptorHandle(source_idx);
				d3d12::SetShaderSRV(cmd_list, 0, source_idx, handle_b_srv);
			}
			
			{
				constexpr unsigned int source_idx = rs_layout::GetHeapLoc(params::bloom_extract_bright, params::BloomExtractBrightE::G_DEPTH);
				auto handle_b_srv = data.out_allocation.GetDescriptorHandle(source_idx);
				d3d12::SetShaderSRV(cmd_list, 0, source_idx, handle_b_srv);
			}

			cmd_list->m_native->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(data.out_source_rt->m_render_targets[frame_idx % versions]));

			d3d12::Dispatch(cmd_list,
				static_cast<int>(std::ceil(n_render_system.m_viewport.m_viewport.Width / 16.f)),
				static_cast<int>(std::ceil(n_render_system.m_viewport.m_viewport.Height / 16.f)),
				1);
		}

		inline void DestroyBloomExtractBrightTask(FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			if (!resize)
			{
				auto& data = fg.GetData<BloomExtractBrightData>(handle);

				// Small hack to force the allocations to go out of scope, which will tell the allocator to free them
				DescriptorAllocation temp1 = std::move(data.out_allocation);
				delete data.out_allocator;
			}
		}

	} /* internal */

	template<typename T, typename T1>
	inline void AddBloomExtractBrightTask(FrameGraph& frame_graph)
	{
		RenderTargetProperties rt_properties
		{
			RenderTargetProperties::IsRenderWindow(false),
			RenderTargetProperties::Width(std::nullopt),
			RenderTargetProperties::Height(std::nullopt),
			RenderTargetProperties::ExecuteResourceState(ResourceState::UNORDERED_ACCESS),
			RenderTargetProperties::FinishedResourceState(ResourceState::COPY_SOURCE),
			RenderTargetProperties::CreateDSVBuffer(false),
			RenderTargetProperties::DSVFormat(Format::UNKNOWN),
			RenderTargetProperties::RTVFormats({ wr::Format::R16G16B16A16_FLOAT}),
			RenderTargetProperties::NumRTVFormats(1),
			RenderTargetProperties::Clear(false),
			RenderTargetProperties::ClearDepth(false),
			RenderTargetProperties::ResolutionScalar(0.5f)
		};

		RenderTaskDesc desc; 
		desc.m_setup_func = [](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::SetupBloomExtractBrightTask<T, T1>(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle) {
			internal::ExecuteBloomExtractBrightTask<T, T1>(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::DestroyBloomExtractBrightTask(fg, handle, resize);
		};
		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		frame_graph.AddTask<BloomExtractBrightData>(desc, L"extract bright");
	}

} /* wr */

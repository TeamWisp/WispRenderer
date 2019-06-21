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
#include "../render_tasks/d3d12_post_processing.hpp"

#include "d3d12_raytracing_task.hpp"

namespace wr
{
	struct DoFBokehPostFilterData
	{
		d3d12::RenderTarget* out_source_rt = nullptr;
		d3d12::PipelineState* out_pipeline = nullptr;
		ID3D12Resource* out_previous = nullptr;
		DescriptorAllocator* out_allocator = nullptr;
		DescriptorAllocation out_allocation;
	};

	namespace internal
	{

		template<typename T>
		inline void SetupDoFBokehPostFilterTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<DoFBokehPostFilterData>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);

			if (!resize)
			{
				data.out_allocator = new DescriptorAllocator(n_render_system, wr::DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV);
				data.out_allocation = data.out_allocator->Allocate(4);
			}

			auto& ps_registry = PipelineRegistry::Get();
			data.out_pipeline = ((d3d12::PipelineState*)ps_registry.Find(pipelines::dof_bokeh_post_filter));

			auto source_rt = data.out_source_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<T>());

			// Source near
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::dof_bokeh_post_filter, params::DoFBokehPostFilterE::SOURCE_NEAR)));
				d3d12::CreateSRVFromSpecificRTV(source_rt, cpu_handle, 0, source_rt->m_create_info.m_rtv_formats[0]);
			}
			// Source far
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::dof_bokeh_post_filter, params::DoFBokehPostFilterE::SOURCE_FAR)));
				d3d12::CreateSRVFromSpecificRTV(source_rt, cpu_handle, 1, source_rt->m_create_info.m_rtv_formats[1]);
			}
			// Destination near
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::dof_bokeh_post_filter, params::DoFBokehPostFilterE::OUTPUT_NEAR)));
				d3d12::CreateUAVFromSpecificRTV(n_render_target, cpu_handle, 0, n_render_target->m_create_info.m_rtv_formats[0]);
			}
			// Destination far
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::dof_bokeh_post_filter, params::DoFBokehPostFilterE::OUTPUT_FAR)));
				d3d12::CreateUAVFromSpecificRTV(n_render_target, cpu_handle, 1, n_render_target->m_create_info.m_rtv_formats[1]);
			}
		
		}

		template<typename T>
		inline void ExecuteDoFBokehPostFilterTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& device = n_render_system.m_device;
			auto& data = fg.GetData<DoFBokehPostFilterData>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);
			auto frame_idx = n_render_system.GetFrameIdx();
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
			const auto viewport = n_render_system.m_viewport;

			auto source_rt = data.out_source_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<T>());

			// Source near
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::dof_bokeh_post_filter, params::DoFBokehPostFilterE::SOURCE_NEAR)));
				d3d12::CreateSRVFromSpecificRTV(source_rt, cpu_handle, 0, source_rt->m_create_info.m_rtv_formats[0]);
			}
			// Source far
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::dof_bokeh_post_filter, params::DoFBokehPostFilterE::SOURCE_FAR)));
				d3d12::CreateSRVFromSpecificRTV(source_rt, cpu_handle, 1, source_rt->m_create_info.m_rtv_formats[1]);
			}
			// Destination near
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::dof_bokeh_post_filter, params::DoFBokehPostFilterE::OUTPUT_NEAR)));
				d3d12::CreateUAVFromSpecificRTV(n_render_target, cpu_handle, 0, n_render_target->m_create_info.m_rtv_formats[0]);
			}
			// Destination far
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::dof_bokeh_post_filter, params::DoFBokehPostFilterE::OUTPUT_FAR)));
				d3d12::CreateUAVFromSpecificRTV(n_render_target, cpu_handle, 1, n_render_target->m_create_info.m_rtv_formats[1]);
			}

			d3d12::BindComputePipeline(cmd_list, data.out_pipeline);

			{
				constexpr unsigned int dest_n_idx = rs_layout::GetHeapLoc(params::dof_bokeh_post_filter, params::DoFBokehPostFilterE::OUTPUT_NEAR);
				auto handle_uav = data.out_allocation.GetDescriptorHandle(dest_n_idx);
				d3d12::SetShaderUAV(cmd_list, 0, dest_n_idx, handle_uav);
			}

			{
				constexpr unsigned int dest_f_idx = rs_layout::GetHeapLoc(params::dof_bokeh_post_filter, params::DoFBokehPostFilterE::OUTPUT_FAR);
				auto handle_uav = data.out_allocation.GetDescriptorHandle(dest_f_idx);
				d3d12::SetShaderUAV(cmd_list, 0, dest_f_idx, handle_uav);
			}

			{
				constexpr unsigned int source_near_idx = rs_layout::GetHeapLoc(params::dof_bokeh_post_filter, params::DoFBokehPostFilterE::SOURCE_NEAR);
				auto handle_m_srv = data.out_allocation.GetDescriptorHandle(source_near_idx);
				d3d12::SetShaderSRV(cmd_list, 0, source_near_idx, handle_m_srv);
			}

			{
				constexpr unsigned int source_far_idx = rs_layout::GetHeapLoc(params::dof_bokeh_post_filter, params::DoFBokehPostFilterE::SOURCE_FAR);
				auto handle_b_srv = data.out_allocation.GetDescriptorHandle(source_far_idx);
				d3d12::SetShaderSRV(cmd_list, 0, source_far_idx, handle_b_srv);
			}

			cmd_list->m_native->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(data.out_source_rt->m_render_targets[frame_idx % versions]));

			//TODO: numthreads is currently hardcoded to half resolution, change when dimensions are done properly
			d3d12::Dispatch(cmd_list,
				static_cast<int>(std::ceil(n_render_system.m_viewport.m_viewport.Width / 16.f)),
				static_cast<int>(std::ceil(n_render_system.m_viewport.m_viewport.Height / 16.f)),
				1);
		}

		inline void DestroyDoFBokehPostFilterTask(FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			if (!resize)
			{
				auto& data = fg.GetData<DoFBokehPostFilterData>(handle);
				delete data.out_allocator;
				data.~DoFBokehPostFilterData();
			}
		}

	} /* internal */

	template<typename T>
	inline void AddDoFBokehPostFilterTask(FrameGraph& frame_graph)
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
			RenderTargetProperties::RTVFormats({ wr::Format::R16G16B16A16_FLOAT,wr::Format::R16G16B16A16_FLOAT }),
			RenderTargetProperties::NumRTVFormats(2),
			RenderTargetProperties::Clear(false),
			RenderTargetProperties::ClearDepth(false),
			RenderTargetProperties::ResolutionScalar(0.5f)
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::SetupDoFBokehPostFilterTask<T>(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle) {
			internal::ExecuteDoFBokehPostFilterTask<T>(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::DestroyDoFBokehPostFilterTask(fg, handle, resize);
		};
		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		frame_graph.AddTask<DoFBokehPostFilterData>(desc, L"DoF Bokeh Post Filter");
	}

} /* wr */

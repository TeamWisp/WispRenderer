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

	struct BloomSettings
	{
		struct Runtime
		{
			float m_sigma = 2.0f;
			float m_bloom_intensity = 1.0f;
			float m_luminance_threshold = 1.0f;
			bool m_enable_bloom = true; 
		};

		Runtime m_runtime;
	};

	struct BloomCompostionData
	{
		d3d12::RenderTarget* out_source_rt = nullptr;
		d3d12::RenderTarget* out_source_bloom_half_rt = nullptr;
		d3d12::RenderTarget* out_source_bloom_quarter_rt = nullptr;
		d3d12::RenderTarget* out_source_bloom_eighth_rt = nullptr;
		d3d12::RenderTarget* out_source_bloom_sixteenth_rt = nullptr;
		d3d12::PipelineState* out_pipeline = nullptr;
		ID3D12Resource* out_previous = nullptr;
		DescriptorAllocator* out_allocator = nullptr;
		DescriptorAllocation out_allocation;

		std::shared_ptr<ConstantBufferPool> bloom_cb_pool;
		D3D12ConstantBufferHandle* cb_handle = nullptr;
	};

	namespace internal
	{
		template<typename T, typename T1>
		inline void SetupBloomCompositionTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<BloomCompostionData>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);

			if (!resize)
			{
				data.out_allocator = new DescriptorAllocator(n_render_system, wr::DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV);
				data.out_allocation = data.out_allocator->Allocate(5);
			}

			auto& ps_registry = PipelineRegistry::Get();
			data.out_pipeline = ((d3d12::PipelineState*)ps_registry.Find(pipelines::bloom_composition));

			auto source_rt = data.out_source_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<T>());
			auto bloom_rt_half = data.out_source_bloom_half_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<T1>());

			// Destination near
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::bloom_composition, params::BloomCompositionE::OUTPUT)));
				d3d12::CreateUAVFromSpecificRTV(n_render_target, cpu_handle, 0, n_render_target->m_create_info.m_rtv_formats[0]);
			}
			// Source main
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::bloom_composition , params::BloomCompositionE::SOURCE_MAIN)));
				d3d12::CreateSRVFromSpecificRTV(source_rt, cpu_handle, 0, source_rt->m_create_info.m_rtv_formats[0]);
			}
			// Source bloom half
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::bloom_composition, params::BloomCompositionE::SOURCE_BLOOM_HALF)));
				d3d12::CreateSRVFromSpecificRTV(bloom_rt_half, cpu_handle, 0, bloom_rt_half->m_create_info.m_rtv_formats[0]);
			}		
			// Source bloom qes
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::bloom_composition, params::BloomCompositionE::SOURCE_BLOOM_QES)));
				d3d12::CreateSRVFromSpecificRTV(bloom_rt_half, cpu_handle, 1, bloom_rt_half->m_create_info.m_rtv_formats[1]);
			}
			
		}

		template<typename T, typename T1>
		inline void ExecuteBloomCompositionTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& device = n_render_system.m_device;
			auto& data = fg.GetData<BloomCompostionData>(handle);
			auto settings = fg.GetSettings<BloomSettings>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);
			auto frame_idx = n_render_system.GetFrameIdx();
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
			const auto viewport = n_render_system.m_viewport;

			d3d12::BindComputePipeline(cmd_list, data.out_pipeline);

			auto source_rt = data.out_source_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<T>());
			auto bloom_rt_half = data.out_source_bloom_half_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<T1>());
			

			// Destination near
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::bloom_composition, params::BloomCompositionE::OUTPUT)));
				d3d12::CreateUAVFromSpecificRTV(n_render_target, cpu_handle, 0, n_render_target->m_create_info.m_rtv_formats[0]);
			}
			// Source main
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::bloom_composition, params::BloomCompositionE::SOURCE_MAIN)));
				d3d12::CreateSRVFromSpecificRTV(source_rt, cpu_handle, 0, source_rt->m_create_info.m_rtv_formats[0]);
			}
			// Source bloom half
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::bloom_composition, params::BloomCompositionE::SOURCE_BLOOM_HALF)));
				d3d12::CreateSRVFromSpecificRTV(bloom_rt_half, cpu_handle, 0, bloom_rt_half->m_create_info.m_rtv_formats[0]);
			}
			// Source bloom qes
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::bloom_composition, params::BloomCompositionE::SOURCE_BLOOM_QES)));
				d3d12::CreateSRVFromSpecificRTV(bloom_rt_half, cpu_handle, 1, bloom_rt_half->m_create_info.m_rtv_formats[1]);
			}



			int enable_dof = settings.m_runtime.m_enable_bloom;
			d3d12::BindCompute32BitConstants(cmd_list, &enable_dof, 1, 0, 1);

			//cmd_list->m_dynamic_descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(0, 0, 3, data.out_allocation.GetDescriptorHandle());
			{
				constexpr unsigned int dest_idx = rs_layout::GetHeapLoc(params::bloom_composition, params::BloomCompositionE::OUTPUT);
				auto handle_uav = data.out_allocation.GetDescriptorHandle(dest_idx);
				d3d12::SetShaderUAV(cmd_list, 0, dest_idx, handle_uav);
			}

			{
				constexpr unsigned int source_main_idx = rs_layout::GetHeapLoc(params::bloom_composition, params::BloomCompositionE::SOURCE_MAIN);
				auto handle_m_srv = data.out_allocation.GetDescriptorHandle(source_main_idx);
				d3d12::SetShaderSRV(cmd_list, 0, source_main_idx, handle_m_srv);
			}

			{
				constexpr unsigned int source_bloom_half_idx = rs_layout::GetHeapLoc(params::bloom_composition, params::BloomCompositionE::SOURCE_BLOOM_HALF);
				auto handle_b_srv = data.out_allocation.GetDescriptorHandle(source_bloom_half_idx);
				d3d12::SetShaderSRV(cmd_list, 0, source_bloom_half_idx, handle_b_srv);
			}

			{
				constexpr unsigned int source_bloom_qes_idx = rs_layout::GetHeapLoc(params::bloom_composition, params::BloomCompositionE::SOURCE_BLOOM_QES);
				auto handle_b_srv = data.out_allocation.GetDescriptorHandle(source_bloom_qes_idx);
				d3d12::SetShaderSRV(cmd_list, 0, source_bloom_qes_idx, handle_b_srv);
			}



			cmd_list->m_native->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(data.out_source_rt->m_render_targets[frame_idx % versions]));

			d3d12::Dispatch(cmd_list,
				static_cast<int>(std::ceil(n_render_system.m_viewport.m_viewport.Width / 16.f)),
				static_cast<int>(std::ceil(n_render_system.m_viewport.m_viewport.Height / 16.f)),
				1);
		}

		inline void DestroyBloomCompositionTask(FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			if (!resize)
			{
				auto& data = fg.GetData<BloomCompostionData>(handle);
				delete data.out_allocator;
			}
		}

	} /* internal */

	template<typename T, typename T1>
	inline void AddBloomCompositionTask(FrameGraph& frame_graph)
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
			RenderTargetProperties::RTVFormats({wr::Format::R16G16B16A16_FLOAT}),
			RenderTargetProperties::NumRTVFormats(1),
			RenderTargetProperties::Clear(false),
			RenderTargetProperties::ClearDepth(false),
			RenderTargetProperties::ResolutionScalar(1.0f)
		};

		RenderTaskDesc desc; 
		desc.m_setup_func = [](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::SetupBloomCompositionTask<T, T1>(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle) {
			internal::ExecuteBloomCompositionTask<T, T1>(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::DestroyBloomCompositionTask(fg, handle, resize);
		};
		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		frame_graph.AddTask<BloomCompostionData>(desc, L"Bloom Composition");
		frame_graph.UpdateSettings<BloomCompostionData>(BloomSettings());
	}

} /* wr */

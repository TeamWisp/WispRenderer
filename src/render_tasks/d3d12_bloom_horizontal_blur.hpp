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

	struct BloomBlurHorizontalData
	{
		d3d12::RenderTarget* out_source_rt = nullptr;
		d3d12::RenderTarget* out_source_bloom_qes_rt = nullptr;
		d3d12::PipelineState* out_pipeline = nullptr;
		ID3D12Resource* out_previous = nullptr;
		DescriptorAllocator* out_allocator = nullptr;
		DescriptorAllocation out_allocation;

		std::shared_ptr<ConstantBufferPool> cb_pool;
		D3D12ConstantBufferHandle* cb_handle = nullptr;
	};

	namespace internal
	{

		struct Bloomblur_CB
		{
			DirectX::XMFLOAT2 blur_dir;
			float _pad = 0.f;
			float sigma;
		};

		template<typename T>
		inline void SetupBloomBlurHorizontalTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<BloomBlurHorizontalData>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);

			if (!resize)
			{
				data.out_allocator = new DescriptorAllocator(n_render_system, wr::DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV);
				data.out_allocation = data.out_allocator->Allocate(4);

				data.cb_pool = rs.CreateConstantBufferPool(2);
				data.cb_handle = static_cast<D3D12ConstantBufferHandle*>(data.cb_pool->Create(sizeof(Bloomblur_CB)));
			}

			auto& ps_registry = PipelineRegistry::Get();
			data.out_pipeline = ((d3d12::PipelineState*)ps_registry.Find(pipelines::bloom_blur_horizontal));

			auto source_rt = data.out_source_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<T>());

			// Output half 
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::bloom_blur_horizontal, params::BloomBlurHorizontalE::OUTPUT)));
				d3d12::CreateUAVFromSpecificRTV(n_render_target, cpu_handle, 0, n_render_target->m_create_info.m_rtv_formats[0]);
			}
			// Output quarter/ eighth / sixteenth
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::bloom_blur_horizontal, params::BloomBlurHorizontalE::OUTPUT_QES)));
				d3d12::CreateUAVFromSpecificRTV(n_render_target, cpu_handle, 1, n_render_target->m_create_info.m_rtv_formats[1]);
			}
			// Source main
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::bloom_blur_horizontal, params::BloomBlurHorizontalE::SOURCE_MAIN)));
				d3d12::CreateSRVFromSpecificRTV(source_rt, cpu_handle, 0, source_rt->m_create_info.m_rtv_formats[0]);
			}
			
		}

		template<typename T>
		inline void ExecuteBloomBlurHorizontalTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& device = n_render_system.m_device;
			auto& data = fg.GetData<BloomBlurHorizontalData>(handle);
			auto settings = fg.GetSettings<BloomSettings>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);
			auto frame_idx = n_render_system.GetFrameIdx();
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
			const auto viewport = n_render_system.m_viewport;

			d3d12::BindComputePipeline(cmd_list, data.out_pipeline);

			auto source_rt = data.out_source_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<T>());

			// Output half 
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::bloom_blur_horizontal, params::BloomBlurHorizontalE::OUTPUT)));
				d3d12::CreateUAVFromSpecificRTV(n_render_target, cpu_handle, 0, n_render_target->m_create_info.m_rtv_formats[0]);
			}
			// Output quarter/ eighth / sixteenth
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::bloom_blur_horizontal, params::BloomBlurHorizontalE::OUTPUT_QES)));
				d3d12::CreateUAVFromSpecificRTV(n_render_target, cpu_handle, 1, n_render_target->m_create_info.m_rtv_formats[1]);
			}
			// Source main
			{
				auto cpu_handle = data.out_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::bloom_blur_horizontal, params::BloomBlurHorizontalE::SOURCE_MAIN)));
				d3d12::CreateSRVFromSpecificRTV(source_rt, cpu_handle, 0, source_rt->m_create_info.m_rtv_formats[0]);
			}

			Bloomblur_CB cb_data;
			cb_data.blur_dir = DirectX::XMFLOAT2(1.0f, 0.0f);
			cb_data.sigma = 2.0f;

			data.cb_handle->m_pool->Update(data.cb_handle, sizeof(Bloomblur_CB), 0, frame_idx, (uint8_t*)& cb_data);
			d3d12::BindComputeConstantBuffer(cmd_list, data.cb_handle->m_native, 1, frame_idx);

			{
				constexpr unsigned int dest_idx = rs_layout::GetHeapLoc(params::bloom_blur_horizontal , params::BloomBlurHorizontalE::OUTPUT);
				auto handle_uav = data.out_allocation.GetDescriptorHandle(dest_idx);
				d3d12::SetShaderUAV(cmd_list, 0, dest_idx, handle_uav);
			}

			{
				constexpr unsigned int dest_idx = rs_layout::GetHeapLoc(params::bloom_blur_horizontal, params::BloomBlurHorizontalE::OUTPUT_QES);
				auto handle_uav = data.out_allocation.GetDescriptorHandle(dest_idx);
				d3d12::SetShaderUAV(cmd_list, 0, dest_idx, handle_uav);
			}

			{
				constexpr unsigned int source_main_idx = rs_layout::GetHeapLoc(params::bloom_blur_horizontal, params::BloomBlurHorizontalE::SOURCE_MAIN);
				auto handle_m_srv = data.out_allocation.GetDescriptorHandle(source_main_idx);
				d3d12::SetShaderSRV(cmd_list, 0, source_main_idx, handle_m_srv);
			}

			

			cmd_list->m_native->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(data.out_source_rt->m_render_targets[frame_idx % versions]));

			float width = n_render_system.m_viewport.m_viewport.Width;
			float height = n_render_system.m_viewport.m_viewport.Height;

			d3d12::Dispatch(cmd_list,
				static_cast<int>(std::ceil(width / 16.f)) + 
				static_cast<int>(std::ceil(width / 2.0f / 16.f)) + 
				static_cast<int>(std::ceil(width / 4 / 16.f)) +
				static_cast<int>(std::ceil(width / 8 / 16.f)),
				static_cast<int>(std::ceil(height / 16.f)) + 
				static_cast<int>(std::ceil(height / 2.0f / 16.f)) + 
				static_cast<int>(std::ceil(height / 4.0f / 16.f)) + 
				static_cast<int>(std::ceil(height / 8.0f / 16.f)),
				1);
		}

		inline void DestroyBloomBlurHorizontalTask(FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			if (!resize)
			{
				auto& data = fg.GetData<BloomBlurHorizontalData>(handle);
				delete data.out_allocator;
				data.~BloomBlurHorizontalData();
			}
		}

	} /* internal */

	template<typename T>
	inline void AddBloomBlurHorizontalTask(FrameGraph& frame_graph)
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
			RenderTargetProperties::RTVFormats({wr::Format::R16G16B16A16_FLOAT,wr::Format::R16G16B16A16_FLOAT}),
			RenderTargetProperties::NumRTVFormats(2),
			RenderTargetProperties::Clear(false),
			RenderTargetProperties::ClearDepth(false),
			RenderTargetProperties::ResolutionScalar(0.5f)
		};

		RenderTaskDesc desc; 
		desc.m_setup_func = [](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::SetupBloomBlurHorizontalTask<T>(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle) {
			internal::ExecuteBloomBlurHorizontalTask<T>(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::DestroyBloomBlurHorizontalTask(fg, handle, resize);
		};
		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		frame_graph.AddTask<BloomBlurHorizontalData>(desc, L"Bloom blur test");
		frame_graph.UpdateSettings<BloomBlurHorizontalData>(BloomSettings());
	}

} /* wr */

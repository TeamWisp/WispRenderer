#pragma once

#include "../frame_graph/frame_graph.hpp"
#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "d3d12_raytracing_task.hpp"
#include "d3d12_deferred_main.hpp"

#ifdef NVIDIA_GAMEWORKS_HBAO
#include <GFSDK_SSAO.h>
#endif

namespace wr
{
	struct HBAOSettings
	{
		struct Runtime
		{
			float m_meters_to_view_space_units = 1; // DistanceInViewSpaceUnits = MetersToViewSpaceUnits * DistanceInMeters
			float m_radius = 2.f; // The AO radius in meters
			float m_bias = 0.1f;  // To hide low-tessellation artifacts // 0.0~0.5
			float m_power_exp = 2.f; // The final AO output is pow(AO, powerExponent) // 1.0~4.0
			bool m_enable_blur = true; // To blur the AO with an edge-preserving blur
			float m_blur_sharpness = 16.f; // The higher, the more the blur preserves edges // 0.0~16.0
		};

		Runtime m_runtime;
	};

	struct HBAOData
	{
		d3d12::RenderTarget* out_deferred_main_rt = nullptr;
		d3d12::DescriptorHeap* out_descriptor_heap_srv = nullptr;
		d3d12::DescriptorHeap* out_descriptor_heap_rtv = nullptr;

#ifdef NVIDIA_GAMEWORKS_HBAO
		GFSDK_SSAO_Context_D3D12* ssao_context = nullptr;
		GFSDK_SSAO_InputData_D3D12 ssao_input_data;
#endif

		d3d12::DescHeapCPUHandle cpu_depth_handle{};
		d3d12::DescHeapGPUHandle gpu_depth_handle{};
		d3d12::DescHeapCPUHandle cpu_normal_handle{};
		d3d12::DescHeapGPUHandle gpu_normal_handle{};
		d3d12::DescHeapCPUHandle cpu_target_handle{};
	};

	namespace internal
	{

		inline void SetupHBAOTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool)
		{
#ifdef NVIDIA_GAMEWORKS_HBAO
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto n_device = n_render_system.m_device;
			auto& data = fg.GetData<HBAOData>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);

			// shader resoruce view heap.
			{
				d3d12::desc::DescriptorHeapDesc heap_desc;
				heap_desc.m_num_descriptors = 2 + GFSDK_SSAO_NUM_DESCRIPTORS_CBV_SRV_UAV_HEAP_D3D12;
				heap_desc.m_type = DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV;
				heap_desc.m_versions = 1;
				data.out_descriptor_heap_srv = d3d12::CreateDescriptorHeap(n_device, heap_desc);

				data.gpu_depth_handle = d3d12::GetGPUHandle(data.out_descriptor_heap_srv, 0);
				data.cpu_depth_handle = d3d12::GetCPUHandle(data.out_descriptor_heap_srv, 0);
				data.gpu_normal_handle = d3d12::GetGPUHandle(data.out_descriptor_heap_srv, 0, 1);
				data.cpu_normal_handle = d3d12::GetCPUHandle(data.out_descriptor_heap_srv, 0, 1);
			}

			// render target view heap.
			{
				d3d12::desc::DescriptorHeapDesc heap_desc;
				heap_desc.m_num_descriptors = GFSDK_SSAO_NUM_DESCRIPTORS_RTV_HEAP_D3D12;
				heap_desc.m_type = DescriptorHeapType::DESC_HEAP_TYPE_RTV;
				heap_desc.m_shader_visible = false;
				heap_desc.m_versions = 1;
				data.out_descriptor_heap_rtv = d3d12::CreateDescriptorHeap(n_device, heap_desc);

				data.cpu_target_handle = d3d12::GetCPUHandle(data.out_descriptor_heap_rtv, 0);
			}

			// depth & normal
			{
				auto deferred_main_rt = data.out_deferred_main_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<DeferredMainTaskData>());
				d3d12::CreateSRVFromSpecificRTV(deferred_main_rt, data.cpu_normal_handle, 1, deferred_main_rt->m_create_info.m_rtv_formats[1]);
				d3d12::CreateSRVFromDSV(deferred_main_rt, data.cpu_depth_handle);
			}

			// target
			{
				n_render_target->m_rtv_descriptor_increment_size = n_device->m_native->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
				n_device->m_native->CreateRenderTargetView(n_render_target->m_render_targets[0], nullptr, data.cpu_target_handle.m_native);
			}

			data.ssao_input_data = {};
			data.ssao_input_data.DepthData.DepthTextureType = GFSDK_SSAO_HARDWARE_DEPTHS;
			data.ssao_input_data.DepthData.FullResDepthTextureSRV.pResource = data.out_deferred_main_rt->m_depth_stencil_buffer;
			data.ssao_input_data.DepthData.FullResDepthTextureSRV.GpuHandle = data.gpu_depth_handle.m_native.ptr;
			data.ssao_input_data.NormalData.Enable = false;
			data.ssao_input_data.NormalData.FullResNormalTextureSRV.pResource = data.out_deferred_main_rt->m_render_targets[1];
			data.ssao_input_data.NormalData.FullResNormalTextureSRV.GpuHandle = data.gpu_normal_handle.m_native.ptr;

			GFSDK_SSAO_CustomHeap custom_heap;
			custom_heap.new_ = ::operator new;
			custom_heap.delete_ = ::operator delete;

			// ao descriptor heap description
			GFSDK_SSAO_DescriptorHeaps_D3D12 DescriptorHeaps;
			DescriptorHeaps.CBV_SRV_UAV.pDescHeap = data.out_descriptor_heap_srv->m_native[0];
			DescriptorHeaps.CBV_SRV_UAV.BaseIndex = 2;
			DescriptorHeaps.RTV.pDescHeap = data.out_descriptor_heap_rtv->m_native[0];
			DescriptorHeaps.RTV.BaseIndex = 0;

			// initialize the ao context
			GFSDK_SSAO_Status status = GFSDK_SSAO_CreateContext_D3D12(n_device->m_native, 1, DescriptorHeaps, &data.ssao_context, &custom_heap);
			if (status != GFSDK_SSAO_Status::GFSDK_SSAO_OK)
			{
				LOGW("Failed to initialize the NVIDIA HBAO+ context.")
			}
#endif
		}

		inline void ExecuteHBAOTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle)
		{
#ifdef NVIDIA_GAMEWORKS_HBAO
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<HBAOData>(handle);
			auto settings = fg.GetSettings<HBAOSettings>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
			const auto viewport = n_render_system.m_viewport;

			data.ssao_input_data.DepthData.ProjectionMatrix.Data = GFSDK_SSAO_Float4x4((const GFSDK_SSAO_FLOAT*)& sg.GetActiveCamera()->m_projection);
			data.ssao_input_data.DepthData.ProjectionMatrix.Layout = GFSDK_SSAO_ROW_MAJOR_ORDER;
			data.ssao_input_data.NormalData.WorldToViewMatrix.Data = GFSDK_SSAO_Float4x4((const GFSDK_SSAO_FLOAT*)& sg.GetActiveCamera()->m_inverse_view);
			data.ssao_input_data.NormalData.WorldToViewMatrix.Layout = GFSDK_SSAO_ROW_MAJOR_ORDER;
			data.ssao_input_data.DepthData.MetersToViewSpaceUnits = settings.m_runtime.m_meters_to_view_space_units;

			// Set the viewport
			data.ssao_input_data.DepthData.Viewport.Enable = true;
			data.ssao_input_data.DepthData.Viewport.Height = static_cast<GFSDK_SSAO_UINT>(viewport.m_viewport.Height);
			data.ssao_input_data.DepthData.Viewport.Width = static_cast<GFSDK_SSAO_UINT>(viewport.m_viewport.Width);
			data.ssao_input_data.DepthData.Viewport.TopLeftX = static_cast<GFSDK_SSAO_UINT>(viewport.m_viewport.TopLeftX);
			data.ssao_input_data.DepthData.Viewport.TopLeftY = static_cast<GFSDK_SSAO_UINT>(viewport.m_viewport.TopLeftY);
			data.ssao_input_data.DepthData.Viewport.MinDepth = static_cast<GFSDK_SSAO_FLOAT>(viewport.m_viewport.MinDepth);
			data.ssao_input_data.DepthData.Viewport.MaxDepth = static_cast<GFSDK_SSAO_FLOAT>(viewport.m_viewport.MaxDepth);

			GFSDK_SSAO_Parameters ao_parameters = {};
			ao_parameters.Radius = settings.m_runtime.m_radius;
			ao_parameters.Bias = settings.m_runtime.m_bias;
			ao_parameters.PowerExponent = settings.m_runtime.m_power_exp;
			ao_parameters.Blur.Enable = settings.m_runtime.m_enable_blur;
			ao_parameters.Blur.Radius = GFSDK_SSAO_BLUR_RADIUS_4;
			ao_parameters.Blur.Sharpness = settings.m_runtime.m_blur_sharpness;
			ao_parameters.EnableDualLayerAO = false;

			GFSDK_SSAO_RenderMask render_mask = GFSDK_SSAO_RENDER_AO;

			GFSDK_SSAO_RenderTargetView_D3D12 rtv {};
			rtv.pResource = n_render_target->m_render_targets[0];
			rtv.CpuHandle = n_render_target->m_rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart().ptr;
			
			GFSDK_SSAO_Output_D3D12 output;
			output.pRenderTargetView = &rtv;

			d3d12::Transition(cmd_list, n_render_target, ResourceState::COPY_SOURCE, ResourceState::RENDER_TARGET);

			d3d12::BindDescriptorHeap(cmd_list, data.out_descriptor_heap_srv, DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV, 0);

			GFSDK_SSAO_Status status = data.ssao_context->RenderAO(n_render_system.m_direct_queue->m_native, cmd_list->m_native, data.ssao_input_data, ao_parameters, output, render_mask);
			if (status != GFSDK_SSAO_Status::GFSDK_SSAO_OK)
			{
				LOGW("Failed to perform NVIDIA HBAO+");
			}

			d3d12::Transition(cmd_list, n_render_target, ResourceState::RENDER_TARGET, ResourceState::COPY_SOURCE);
#endif
		}

		inline void DestroyHBAOTask(FrameGraph& fg, RenderTaskHandle handle, bool)
		{
#ifdef NVIDIA_GAMEWORKS_HBAO
			auto& data = fg.GetData<HBAOData>(handle);
			d3d12::Destroy(data.out_descriptor_heap_srv);
			d3d12::Destroy(data.out_descriptor_heap_rtv);

			delete data.ssao_context;
#endif
		}

	} /* internal */

	inline void AddHBAOTask(FrameGraph& frame_graph)
	{
#ifndef NVIDIA_GAMEWORKS_HBAO
		LOGW("HBAO+ task has been added to the frame graph. But `NVIDIA_GAMEWORKS_HBAO` is not defined.");
#endif
		RenderTargetProperties rt_properties
		{
			RenderTargetProperties::IsRenderWindow(false),
			RenderTargetProperties::Width(std::nullopt),
			RenderTargetProperties::Height(std::nullopt),
			RenderTargetProperties::ExecuteResourceState(ResourceState::RENDER_TARGET),
			RenderTargetProperties::FinishedResourceState(ResourceState::COPY_SOURCE),
			RenderTargetProperties::CreateDSVBuffer(false),
			RenderTargetProperties::DSVFormat(Format::UNKNOWN),
			RenderTargetProperties::RTVFormats({ d3d12::settings::back_buffer_format }),
			RenderTargetProperties::NumRTVFormats(1),
			RenderTargetProperties::Clear(false),
			RenderTargetProperties::ClearDepth(false),
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::SetupHBAOTask(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle) {
			internal::ExecuteHBAOTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::DestroyHBAOTask(fg, handle, resize);
		};

		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		frame_graph.AddTask<HBAOData>(desc, L"NVIDIA HBAO+", FG_DEPS<DeferredMainTaskData>());
		frame_graph.UpdateSettings<HBAOData>(HBAOSettings());
	}

} /* wr */

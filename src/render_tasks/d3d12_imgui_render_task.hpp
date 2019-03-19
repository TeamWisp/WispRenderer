#pragma once

#include "../window.hpp"
#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../imgui/ImGuizmo.h"

#include "../imgui/imgui.hpp"
#include "../imgui/imgui_impl_win32.hpp"
#include "../imgui/imgui_impl_dx12.hpp"

namespace wr
{

	struct ImGuiTaskData
	{
		std::function<void(ImTextureID)> in_imgui_func;
		d3d12::DescriptorHeap* out_descriptor_heap;
	};
	
	namespace internal
	{

		inline void SetupImGuiTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<ImGuiTaskData>(handle);

			if (!n_render_system.m_window.has_value())
			{
				LOGC("Tried using imgui without a window!");
			}

			if (resize)
			{
				ImGui_ImplDX12_CreateDeviceObjects();
				return;
			}

			if (ImGui_ImplDX12_IsInitialized())
			{
				data.out_descriptor_heap = ImGui_ImplDX12_GetDescriptorHeap();
				return;
			}

			d3d12::desc::DescriptorHeapDesc heap_desc;
			heap_desc.m_num_descriptors = 2;
			heap_desc.m_shader_visible = true;
			heap_desc.m_type = DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV;
			data.out_descriptor_heap = d3d12::CreateDescriptorHeap(n_render_system.m_device, heap_desc);
			SetName(data.out_descriptor_heap, L"ImGui Descriptor Heap");

			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO(); (void)io;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
			io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows (FIXME: Currently broken in DX12 back-end, need some work!)
			io.ConfigDockingWithShift = true;

			ImGui_ImplWin32_Init(n_render_system.m_window.value()->GetWindowHandle());
			ImGui_ImplDX12_Init(n_render_system.m_device->m_native,
				d3d12::settings::num_back_buffers,
				(DXGI_FORMAT)d3d12::settings::back_buffer_format,
				d3d12::GetCPUHandle(data.out_descriptor_heap, 0 /* TODO: Solve versioning for ImGui */).m_native,
				d3d12::GetGPUHandle(data.out_descriptor_heap, 0 /* TODO: Solve versioning for ImGui */).m_native,
				data.out_descriptor_heap);

			ImGui::StyleColorsCherry();
		}

		template<typename T>
		inline void ExecuteImGuiTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<ImGuiTaskData>(handle);
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
			auto render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);

			// Temp rendering
			if (n_render_system.m_render_window.has_value())
			{
				auto frame_idx = n_render_system.GetFrameIdx();

				// Create handle to the render target you want to display. and put it in descriptor slot 2.
				auto display_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<T>());
				constexpr unsigned int source_idx = rs_layout::GetHeapLoc(params::post_processing, params::PostProcessingE::SOURCE);
				auto cpu_handle = d3d12::GetCPUHandle(data.out_descriptor_heap, frame_idx, 1);
				d3d12::CreateSRVFromSpecificRTV(display_rt, cpu_handle, 0, display_rt->m_create_info.m_rtv_formats[frame_idx]);

				// Prepare imgui
				ImGui_ImplDX12_NewFrame();
				ImGui_ImplWin32_NewFrame();
				ImGui::NewFrame();
				ImGuizmo::BeginFrame();

				data.in_imgui_func(ImTextureID(d3d12::GetGPUHandle(data.out_descriptor_heap, frame_idx, 1).m_native.ptr));

				// Render imgui
				
				//EXCEPTION CODE START
				d3d12::BindDescriptorHeap(cmd_list, data.out_descriptor_heap, data.out_descriptor_heap->m_create_info.m_type, n_render_system.GetFrameIdx());
				d3d12::BindDescriptorHeaps(cmd_list, n_render_system.GetFrameIdx());

				for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
				{
					cmd_list->m_dynamic_descriptor_heaps[i]->CommitStagedDescriptorsForDraw(*cmd_list);
				}
				//EXCEPTION CODE END. Don't copy this outside of imgui render task. Since imgui doesn't use our Draw functions, this is needed to make it work with the dynamic descriptor heaps.
				
				ImGui::Render();
				ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmd_list->m_native);

				// Update and Render additional Platform Windows (Beta-Viewport)
				if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
				{
					ImGui::UpdatePlatformWindows();
					ImGui::RenderPlatformWindowsDefault();
				}
			}
		}

		inline void DestroyImGuiTask(FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			if (resize)
			{
				ImGui_ImplDX12_InvalidateDeviceObjects();
			}
			else if (ImGui_ImplDX12_IsInitialized())
			{
				ImGui_ImplDX12_Shutdown();
				ImGui_ImplWin32_Shutdown();
				ImGui::DestroyContext();
			}
		}

	} /* internal */

	template<typename T>
	[[nodiscard]] inline RenderTaskDesc GetImGuiTask(std::function<void(ImTextureID)> imgui_func)
	{
		RenderTargetProperties rt_properties
		{
			RenderTargetProperties::IsRenderWindow(true),
			RenderTargetProperties::Width(std::nullopt),
			RenderTargetProperties::Height(std::nullopt),
			RenderTargetProperties::ExecuteResourceState(std::nullopt),
			RenderTargetProperties::FinishedResourceState(std::nullopt),
			RenderTargetProperties::CreateDSVBuffer(false),
			RenderTargetProperties::DSVFormat(Format::UNKNOWN),
			RenderTargetProperties::RTVFormats({ Format::R16G16B16A16_UNORM }),
			RenderTargetProperties::NumRTVFormats(1),
			RenderTargetProperties::Clear(false),
			RenderTargetProperties::ClearDepth(false)
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [imgui_func](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			fg.GetData<ImGuiTaskData>(handle).in_imgui_func = imgui_func;
			internal::SetupImGuiTask(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle) {
			internal::ExecuteImGuiTask<T>(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::DestroyImGuiTask(fg, handle, resize);
		};
		desc.m_name = "Dear ImGui";
		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::DIRECT;
		desc.m_allow_multithreading = false;
		desc.m_post_processing = true;

		return desc;
	}

} /* wr */
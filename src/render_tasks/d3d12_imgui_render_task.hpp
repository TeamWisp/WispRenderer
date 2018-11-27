#pragma once

#include "../window.hpp"
#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../frame_graph/render_task.hpp"
#include "../frame_graph/frame_graph.hpp"

#include "../imgui/imgui.hpp"
#include "../imgui/imgui_impl_win32.hpp"
#include "../imgui/imgui_impl_dx12.hpp"

namespace wr
{

	struct ImGuiTaskData
	{
		std::function<void()> in_imgui_func;
		d3d12::DescriptorHeap* out_descriptor_heap;
	};

	using ImGuiRenderTask_t = RenderTask<ImGuiTaskData>;
	
	namespace internal
	{

		inline void SetupImGuiTask(RenderSystem & render_system, ImGuiRenderTask_t & task, ImGuiTaskData & data)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);

			if (!n_render_system.m_window.has_value())
			{
				LOGC("Tried using imgui without a window!");
			}

			d3d12::desc::DescriptorHeapDesc heap_desc;
			heap_desc.m_num_descriptors = 1;
			heap_desc.m_shader_visible = true;
			heap_desc.m_type = DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV;
			data.out_descriptor_heap = d3d12::CreateDescriptorHeap(n_render_system.m_device, heap_desc);

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
				d3d12::GetCPUHandle(data.out_descriptor_heap, 0 /* TODO: Version here? */).m_native,
				d3d12::GetGPUHandle(data.out_descriptor_heap, 0 /* TODO: Version here? */).m_native);

			ImGui::StyleColorsCherry();
		}

		inline void ExecuteImGuiTask(RenderSystem & render_system, ImGuiRenderTask_t & task, SceneGraph & scene_graph, ImGuiTaskData & data)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);

			// Temp rendering
			if (n_render_system.m_render_window.has_value())
			{
				const auto cmd_list = task.GetCommandList<D3D12CommandList>().first;

				// Prepare imgui
				ImGui_ImplDX12_NewFrame();
				ImGui_ImplWin32_NewFrame();
				ImGui::NewFrame();

				data.in_imgui_func();

				// Render imgui
				d3d12::BindDescriptorHeaps(cmd_list, { data.out_descriptor_heap }, n_render_system.GetFrameIdx());
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

		inline void ResizeImGuiTask(ImGuiRenderTask_t & task, ImGuiTaskData & data, std::uint32_t width, std::uint32_t height)
		{
			ImGui_ImplDX12_InvalidateDeviceObjects();
			ImGui_ImplDX12_CreateDeviceObjects();
		}

		inline void DestroyImGuiTask(ImGuiRenderTask_t & task, ImGuiTaskData& data)
		{
			ImGui_ImplDX12_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
		}

	} /* internal */
	

	//! Used to create a new defferred task.
	[[nodiscard]] inline std::unique_ptr<ImGuiRenderTask_t> GetImGuiTask(std::function<void()> imgui_func)
	{
		auto ptr = std::make_unique<ImGuiRenderTask_t>(nullptr, "ImGui Render Task", RenderTaskType::DIRECT, false,
			RenderTargetProperties {
				true,
				std::nullopt,
				std::nullopt,
				std::nullopt,
				std::nullopt,
				false,
				Format::UNKNOWN,
				{ Format::R8G8B8A8_UNORM },
				1,
				false,
				false
			},
			[imgui_func](RenderSystem & render_system, ImGuiRenderTask_t & task, ImGuiTaskData & data) { data.in_imgui_func = imgui_func; internal::SetupImGuiTask(render_system, task, data); },
			[](RenderSystem & render_system, ImGuiRenderTask_t & task, SceneGraph & scene_graph, ImGuiTaskData & data) { internal::ExecuteImGuiTask(render_system, task, scene_graph, data); },
			[](RenderSystem & render_system, ImGuiRenderTask_t & task, ImGuiTaskData & data, std::uint32_t width, std::uint32_t height) { internal::ResizeImGuiTask(task, data, width, height); },
			[](ImGuiRenderTask_t & task, ImGuiTaskData & data) { internal::DestroyImGuiTask(task, data); }
		);

		return ptr;
	}

} /* wr */
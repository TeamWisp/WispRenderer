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
	
	namespace internal
	{

		inline void SetupImGuiTask(RenderSystem & render_system, RenderTask<ImGuiTaskData> & task, ImGuiTaskData & data)
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

			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO(); (void)io;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
			io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows (FIXME: Currently broken in DX12 back-end, need some work!)
			io.ConfigDockingWithShift = true;

			ImGui_ImplWin32_Init(n_render_system.m_window.value()->GetWindowHandle());
			ImGui_ImplDX12_Init(n_render_system.m_device->m_native,
				d3d12::settings::num_back_buffers,
				d3d12::settings::back_buffer_format,
				d3d12::GetCPUHandle(data.out_descriptor_heap).m_native,
				d3d12::GetGPUHandle(data.out_descriptor_heap).m_native);

			ImGui::StyleColorsLight();
		}

		inline void ExecuteImGuiTask(RenderSystem & render_system, RenderTask<ImGuiTaskData> & task, SceneGraph & scene_graph, ImGuiTaskData & data)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);

			// Temp rendering
			if (n_render_system.m_render_window.has_value())
			{
				const auto cmd_list = n_render_system.m_direct_cmd_list;
				const auto queue = n_render_system.m_direct_queue;
				const auto render_window = n_render_system.GetRenderWindow();
				const auto fences = n_render_system.m_fences;
				const auto pso = n_render_system.m_pipeline_state;
				const auto viewport = n_render_system.m_viewport;
				const auto device = n_render_system.m_device;
				const auto frame_idx = render_window->m_frame_idx;

				d3d12::WaitFor(fences[frame_idx]);

				d3d12::Begin(cmd_list, frame_idx);
				d3d12::Transition(cmd_list, render_window, frame_idx, ResourceState::PRESENT, ResourceState::RENDER_TARGET);

				d3d12::BindRenderTargetVersioned(cmd_list, render_window, frame_idx, true, true);

				// Prepare imgui
				ImGui_ImplDX12_NewFrame();
				ImGui_ImplWin32_NewFrame();
				ImGui::NewFrame();

				data.in_imgui_func();

				// Render imgui
				d3d12::BindDescriptorHeaps(cmd_list, { data.out_descriptor_heap });
				ImGui::Render();
				ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmd_list->m_native);

				d3d12::Transition(cmd_list, render_window, frame_idx, ResourceState::RENDER_TARGET, ResourceState::PRESENT);
				d3d12::End(cmd_list);

				d3d12::Execute(queue, { cmd_list }, fences[frame_idx]);
				d3d12::Signal(fences[frame_idx], queue);

				// Update and Render additional Platform Windows (Beta-Viewport)
				if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
				{
					ImGui::UpdatePlatformWindows();
					ImGui::RenderPlatformWindowsDefault();
				}

				d3d12::Present(render_window, queue, device);
			}
		}

	} /* internal */
	

	//! Used to create a new defferred task.
	[[nodiscard]] inline std::unique_ptr<RenderTask<ImGuiTaskData>> GetImGuiTask(std::function<void()> imgui_func)
	{
		auto ptr = std::make_unique<RenderTask<ImGuiTaskData>>(nullptr, "ImGui Render Task",
			[imgui_func](RenderSystem & render_system, RenderTask<ImGuiTaskData> & task, ImGuiTaskData & data) { data.in_imgui_func = imgui_func; internal::SetupImGuiTask(render_system, task, data); },
			[](RenderSystem & render_system, RenderTask<ImGuiTaskData> & task, SceneGraph & scene_graph, ImGuiTaskData & data) { internal::ExecuteImGuiTask(render_system, task, scene_graph, data); });

		return ptr;
	}

} /* wr */
#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_resource_pool_material.hpp"
#include "../frame_graph/render_task.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../scene_graph/camera_node.hpp"

namespace wr
{

	struct DeferredTaskData
	{
		d3d12::Shader* out_vs;
		d3d12::Shader* out_ps;
	};
	
	namespace internal
	{

		inline void SetupDeferredTask(RenderSystem & render_system, RenderTask<DeferredTaskData> & task, DeferredTaskData & data)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);
		}

		inline void ExecuteDeferredTask(RenderSystem & render_system, RenderTask<DeferredTaskData> & task, SceneGraph & scene_graph, DeferredTaskData & data)
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
				d3d12::Transition(cmd_list, render_window, frame_idx, d3d12::ResourceState::PRESENT, d3d12::ResourceState::RENDER_TARGET);

				d3d12::BindPipeline(cmd_list, pso);
				d3d12::SetPrimitiveTopology(cmd_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
				d3d12::BindRenderTargetVersioned(cmd_list, render_window, frame_idx, true, true);
				d3d12::BindViewport(cmd_list, viewport);

				auto d3d12_cb_handle = static_cast<D3D12ConstantBufferHandle*>(scene_graph.GetActiveCamera()->m_camera_cb);
				d3d12::BindConstantBuffer(cmd_list, d3d12_cb_handle->m_native, 0, frame_idx);

				n_render_system.RenderSceneGraph(scene_graph);

				d3d12::Transition(cmd_list, render_window, frame_idx, d3d12::ResourceState::RENDER_TARGET, d3d12::ResourceState::PRESENT);
				d3d12::End(cmd_list);

				d3d12::Execute(queue, { cmd_list }, fences[frame_idx]);
				d3d12::Signal(fences[frame_idx], queue);
				d3d12::Present(render_window, queue, device);
			}
		}

	} /* internal */
	

	//! Used to create a new defferred task.
	[[nodiscard]] inline std::unique_ptr<RenderTask<DeferredTaskData>> GetTestTask()
	{
		auto ptr = std::make_unique<RenderTask<DeferredTaskData>>(nullptr, "Deferred Render Task",
			[](RenderSystem & render_system, RenderTask<DeferredTaskData> & task, DeferredTaskData & data) { internal::SetupDeferredTask(render_system, task, data); },
			[](RenderSystem & render_system, RenderTask<DeferredTaskData> & task, SceneGraph & scene_graph, DeferredTaskData & data) { internal::ExecuteDeferredTask(render_system, task, scene_graph, data); });

		return ptr;
	}

} /* wr */
#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_resource_pool.hpp"
#include "../frame_graph/render_task.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../scene_graph/camera_node.hpp"
#include "../d3d12/d3d12_pipeline_registry.hpp"
#include "../engine_registry.hpp"

#include "d3d12_imgui_render_task.hpp"

namespace wr
{

	struct DeferredTaskData
	{
		D3D12Pipeline* in_pipeline;
	};
	using TestRenderTask_t = RenderTask<DeferredTaskData>;
	
	namespace internal
	{

		inline void SetupDeferredTask(RenderSystem & render_system, TestRenderTask_t & task, DeferredTaskData & data)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);

			auto& ps_registry = PipelineRegistry::Get();
			data.in_pipeline = (D3D12Pipeline*)ps_registry.Find(pipelines::basic);
		}

		inline void ExecuteDeferredTask(RenderSystem & render_system, TestRenderTask_t & task, SceneGraph & scene_graph, DeferredTaskData & data)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);

			auto& datao = task.GetFrameGraph()->GetData<wr::ImGuiRenderTask_t>();

			// Temp rendering
			if (n_render_system.m_render_window.has_value())
			{
				const auto cmd_list = task.GetCommandList<D3D12CommandList>().first;
				const auto viewport = n_render_system.m_viewport;
				const auto frame_idx = n_render_system.GetRenderWindow()->m_frame_idx;

				d3d12::BindViewport(cmd_list, viewport);
				d3d12::BindPipeline(cmd_list, data.in_pipeline->m_native);
				d3d12::SetPrimitiveTopology(cmd_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				auto d3d12_cb_handle = static_cast<D3D12ConstantBufferHandle*>(scene_graph.GetActiveCamera()->m_camera_cb);
				d3d12::BindConstantBuffer(cmd_list, d3d12_cb_handle->m_native, 0, frame_idx);

				n_render_system.RenderSceneGraph(scene_graph, cmd_list);
			}
		}

		inline void DestroyTestTask(TestRenderTask_t & task, DeferredTaskData& data)
		{
		}

	} /* internal */
	

	//! Used to create a new defferred task.
	[[nodiscard]] inline std::unique_ptr<TestRenderTask_t> GetTestTask()
	{
		auto ptr = std::make_unique<TestRenderTask_t>(nullptr, "Deferred Render Task", RenderTaskType::DIRECT, true,
			RenderTargetProperties{
				true,
				std::nullopt,
				std::nullopt,
				true,
				Format::D32_FLOAT,
				{ Format::R8G8B8A8_UNORM },
				1,
				true,
				true
			},
			[](RenderSystem & render_system, TestRenderTask_t & task, DeferredTaskData & data) { internal::SetupDeferredTask(render_system, task, data); },
			[](RenderSystem & render_system, TestRenderTask_t & task, SceneGraph & scene_graph, DeferredTaskData & data) { internal::ExecuteDeferredTask(render_system, task, scene_graph, data); },
			[](TestRenderTask_t & task, DeferredTaskData & data) { internal::DestroyTestTask(task, data); }
		);

		return ptr;
	}

} /* wr */
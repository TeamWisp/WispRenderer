#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_resource_pool.hpp"
#include "../d3d12/d3d12_resource_pool_constant_buffer.hpp"
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
				const auto cmd_list = task.GetCommandList<D3D12CommandList>().first;
				const auto pso = n_render_system.m_pipeline_state;
				const auto viewport = n_render_system.m_viewport;
				const auto frame_idx = n_render_system.GetRenderWindow()->m_frame_idx;

				d3d12::BindViewport(cmd_list, viewport);
				d3d12::BindPipeline(cmd_list, pso);
				d3d12::SetPrimitiveTopology(cmd_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

				cmd_list->m_native->SetGraphicsRootSignature(pso->m_root_signature->m_native);

				auto d3d12_cb_handle = static_cast<D3D12ConstantBufferHandle*>(scene_graph.GetActiveCamera()->m_camera_cb);
				d3d12::BindConstantBuffer(cmd_list, d3d12_cb_handle->m_native, 0, frame_idx);

				n_render_system.RenderSceneGraph(scene_graph, cmd_list);
			}
		}

		inline void DestroyTestTask(RenderTask<DeferredTaskData> & task, DeferredTaskData& data)
		{
		}

	} /* internal */
	

	//! Used to create a new defferred task.
	[[nodiscard]] inline std::unique_ptr<RenderTask<DeferredTaskData>> GetTestTask()
	{
		auto ptr = std::make_unique<RenderTask<DeferredTaskData>>(nullptr, "Deferred Render Task", RenderTaskType::DIRECT,
			RenderTargetProperties{
				true,
				std::nullopt,
				std::nullopt,
				false,
				Format::UNKNOWN,
				{ Format::R8G8B8A8_UNORM },
				1,
				true,
				true
			},
			[](RenderSystem & render_system, RenderTask<DeferredTaskData> & task, DeferredTaskData & data) { internal::SetupDeferredTask(render_system, task, data); },
			[](RenderSystem & render_system, RenderTask<DeferredTaskData> & task, SceneGraph & scene_graph, DeferredTaskData & data) { internal::ExecuteDeferredTask(render_system, task, scene_graph, data); },
			[](RenderTask<DeferredTaskData> & task, DeferredTaskData & data) { internal::DestroyTestTask(task, data); }
		);

		return ptr;
	}

} /* wr */
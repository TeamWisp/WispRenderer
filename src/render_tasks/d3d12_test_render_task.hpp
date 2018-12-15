#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
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
		}

		inline void ExecuteDeferredTask(RenderSystem & render_system, TestRenderTask_t & task, SceneGraph & scene_graph, DeferredTaskData & data)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);
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
				RenderTargetProperties::IsRenderWindow(true),
				RenderTargetProperties::Width(std::nullopt),
				RenderTargetProperties::Height(std::nullopt),
				RenderTargetProperties::ExecuteResourceState(std::nullopt),
				RenderTargetProperties::FinishedResourceState(std::nullopt),
				RenderTargetProperties::CreateDSVBuffer(true),
				RenderTargetProperties::DSVFormat(Format::D32_FLOAT),
				RenderTargetProperties::RTVFormats({ Format::R8G8B8A8_UNORM }),
				RenderTargetProperties::NumRTVFormats(1),
			},
			[](RenderSystem & render_system, TestRenderTask_t & task, DeferredTaskData & data, bool) { internal::SetupDeferredTask(render_system, task, data); },
			[](RenderSystem & render_system, TestRenderTask_t & task, SceneGraph & scene_graph, DeferredTaskData & data) { internal::ExecuteDeferredTask(render_system, task, scene_graph, data); },
			[](TestRenderTask_t & task, DeferredTaskData & data, bool) { internal::DestroyTestTask(task, data); }
		);

		return ptr;
	}

} /* wr */
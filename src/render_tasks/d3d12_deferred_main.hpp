#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "../d3d12/d3d12_structured_buffer_pool.hpp"
#include "../d3d12/d3d12_model_pool.hpp"
#include "../d3d12/d3d12_resource_pool_texture.hpp"
#include "../frame_graph/render_task.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../scene_graph/camera_node.hpp"
#include "../d3d12/d3d12_pipeline_registry.hpp"
#include "../engine_registry.hpp"

#include "../platform_independend_structs.hpp"
#include "d3d12_imgui_render_task.hpp"
#include "../scene_graph/camera_node.hpp"

namespace wr
{

	struct DeferredMainTaskData
	{
		D3D12Pipeline* in_pipeline;
	};
	using DeferredMainRenderTask_t = RenderTask<DeferredMainTaskData>;
	
	namespace internal
	{

		inline void SetupDeferredTask(RenderSystem & render_system, DeferredMainRenderTask_t & task, DeferredMainTaskData & data)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);

			auto& ps_registry = PipelineRegistry::Get();
			data.in_pipeline = (D3D12Pipeline*)ps_registry.Find(pipelines::basic_deferred);

			const auto cmd_list = task.GetCommandList<d3d12::CommandList>().first;
			auto render_target = task.GetRenderTarget<d3d12::RenderTarget>();
		}

		inline void ExecuteDeferredTask(RenderSystem & render_system, DeferredMainRenderTask_t & task, SceneGraph & scene_graph, DeferredMainTaskData & data)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);

			if (n_render_system.m_render_window.has_value())
			{
				auto cmd_list = task.GetCommandList<d3d12::CommandList>().first;
				const auto viewport = n_render_system.m_viewport;
				const auto frame_idx = n_render_system.GetRenderWindow()->m_frame_idx;
				auto render_target = task.GetRenderTarget<d3d12::RenderTarget>();

				d3d12::BindViewport(cmd_list, viewport);
				d3d12::BindPipeline(cmd_list, data.in_pipeline->m_native);
				d3d12::SetPrimitiveTopology(cmd_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				auto d3d12_cb_handle = static_cast<D3D12ConstantBufferHandle*>(scene_graph.GetActiveCamera()->m_camera_cb);
				d3d12::BindConstantBuffer(cmd_list, d3d12_cb_handle->m_native, 0, frame_idx);

				scene_graph.Render(cmd_list, scene_graph.GetActiveCamera().get());
			}
		}

		inline void DestroyTestTask(DeferredMainRenderTask_t & task, DeferredMainTaskData& data)
		{
		}

	} /* internal */
	

	//! Used to create a new defferred task.
	[[nodiscard]] inline std::unique_ptr<DeferredMainRenderTask_t> GetDeferredMainTask()
	{
		auto ptr = std::make_unique<DeferredMainRenderTask_t>(nullptr, "Deferred Render Task", RenderTaskType::DIRECT, true,
			RenderTargetProperties{
				RenderTargetProperties::IsRenderWindow(false),
				RenderTargetProperties::Width(std::nullopt),
				RenderTargetProperties::Height(std::nullopt),
				RenderTargetProperties::ExecuteResourceState(ResourceState::RENDER_TARGET),
				RenderTargetProperties::FinishedResourceState(ResourceState::NON_PIXEL_SHADER_RESOURCE),
				RenderTargetProperties::CreateDSVBuffer(true),
				RenderTargetProperties::DSVFormat(Format::D32_FLOAT),
				RenderTargetProperties::RTVFormats({ Format::R32G32B32A32_FLOAT, Format::R32G32B32A32_FLOAT }),
				RenderTargetProperties::NumRTVFormats(2),
				RenderTargetProperties::Clear(true),
				RenderTargetProperties::ClearDepth(true),
			},
			[](RenderSystem & render_system, DeferredMainRenderTask_t & task, DeferredMainTaskData & data, bool) { internal::SetupDeferredTask(render_system, task, data); },
			[](RenderSystem & render_system, DeferredMainRenderTask_t & task, SceneGraph & scene_graph, DeferredMainTaskData & data) { internal::ExecuteDeferredTask(render_system, task, scene_graph, data); },
			[](DeferredMainRenderTask_t & task, DeferredMainTaskData & data, bool) { internal::DestroyTestTask(task, data); }
		);

		return ptr;
	}

} /* wr */
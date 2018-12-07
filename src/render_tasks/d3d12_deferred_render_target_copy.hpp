#pragma once
#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "../d3d12/d3d12_structured_buffer_pool.hpp"
#include "../frame_graph/render_task.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../scene_graph/camera_node.hpp"
#include "../d3d12/d3d12_pipeline_registry.hpp"
#include "../engine_registry.hpp"

#include "../render_tasks/d3d12_deferred_composition.hpp"

namespace wr
{
	struct DeferredRenderTargetCopyTaskData
	{
		d3d12::RenderTarget* out_deferred_composition_rt;
	};
	using DeferredRenderTargetCopyRenderTask_t = RenderTask<DeferredRenderTargetCopyTaskData>;

	namespace internal
	{

		inline void RecordDrawCommands(D3D12RenderSystem& render_system, d3d12::CommandList* cmd_list, d3d12::HeapResource* camera_cb, DeferredRenderTargetCopyTaskData const & data, unsigned int frame_idx)
		{
			
		}

		inline void SetupDeferredTask(RenderSystem & render_system, DeferredRenderTargetCopyRenderTask_t & task, DeferredRenderTargetCopyTaskData & data)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);
			auto* fg = task.GetFrameGraph();

			data.out_deferred_composition_rt = static_cast<D3D12RenderTarget*>(fg->GetData<DeferredCompositionTaskData>().m_render_target);
		}

		inline void ExecuteDeferredTask(RenderSystem & render_system, DeferredRenderTargetCopyRenderTask_t& task, SceneGraph & scene_graph, DeferredRenderTargetCopyTaskData& data)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);

			if (n_render_system.m_render_window.has_value())
			{
				const auto cmd_list = task.GetCommandList<D3D12CommandList>().first;
				const auto viewport = n_render_system.m_viewport;
				const auto camera_cb = static_cast<D3D12ConstantBufferHandle*>(scene_graph.GetActiveCamera()->m_camera_cb);
				const auto frame_idx = n_render_system.GetFrameIdx();

				D3D12RenderTarget* render_target = task.GetRenderTarget<D3D12RenderTarget>();

				D3D12_TEXTURE_COPY_LOCATION dst = {};
				dst.pResource = render_target->m_render_targets[frame_idx % render_target->m_render_targets.size()];
				dst.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
				dst.SubresourceIndex = 0;

				D3D12_TEXTURE_COPY_LOCATION src = {};
				src.pResource = data.out_deferred_composition_rt->m_render_targets[0];
				src.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
				src.SubresourceIndex = 0;

				cmd_list->m_native->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
			}
		}


		inline void ResizeDeferredTask(DeferredRenderTargetCopyRenderTask_t & task, DeferredRenderTargetCopyTaskData& data, std::uint32_t width, std::uint32_t height)
		{

		}

		inline void DestroyTestTask(DeferredRenderTargetCopyRenderTask_t& task, DeferredRenderTargetCopyTaskData& data)
		{
			
		}

	} /* internal */


	//! Used to create a new deferred task.
	[[nodiscard]] inline std::unique_ptr<DeferredRenderTargetCopyRenderTask_t> GetDeferredRenderTargetCopyTask()
	{
		auto ptr = std::make_unique<DeferredRenderTargetCopyRenderTask_t>(nullptr, "Deferred Render Task", RenderTaskType::COPY, true,
			RenderTargetProperties{
				true,
				std::nullopt,
				std::nullopt,
				ResourceState::COPY_DEST,
				ResourceState::PRESENT,
				false,
				Format::UNKNOWN,
				{ Format::R8G8B8A8_UNORM },
				1,
				true,
				true
			},
			[](RenderSystem & render_system, DeferredRenderTargetCopyRenderTask_t & task, DeferredRenderTargetCopyTaskData& data, bool) { internal::SetupDeferredTask(render_system, task, data); },
			[](RenderSystem & render_system, DeferredRenderTargetCopyRenderTask_t& task, SceneGraph & scene_graph, DeferredRenderTargetCopyTaskData& data) { internal::ExecuteDeferredTask(render_system, task, scene_graph, data); },
			[](DeferredRenderTargetCopyRenderTask_t & task, DeferredRenderTargetCopyTaskData& data, bool) { internal::DestroyTestTask(task, data); }
		);

		return ptr;
	}

} /* wr */

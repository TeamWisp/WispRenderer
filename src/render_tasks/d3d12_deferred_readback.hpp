#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../frame_graph/frame_graph.hpp"

namespace wr
{
	struct RenderTargetReadBackTaskData
	{
		d3d12::RenderTarget* predecessor_render_target;
	};

	namespace internal
	{
		template<typename T>
		inline void SetupReadBackTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle)
		{
			auto& data = fg.GetData<RenderTargetReadBackTaskData>(handle);
			data.predecessor_render_target = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<T>());
		}

		inline void ExecuteReadBackTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<RenderTargetReadBackTaskData>(handle);
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);

			const auto frame_idx = n_render_system.GetFrameIdx();

			D3D12_RESOURCE_BARRIER output_buffer_resource_barrier
			{
				CD3DX12_RESOURCE_BARRIER::Transition(
					data.predecessor_render_target->m_render_targets[0],
					D3D12_RESOURCE_STATE_COPY_DEST,
					D3D12_RESOURCE_STATE_COPY_SOURCE)
			};

			cmd_list->m_native->ResourceBarrier(1, &output_buffer_resource_barrier);

			// Copy the GPU data to a CPU-visible resource
			cmd_list->m_native->CopyResource(DEST, data.predecessor_render_target->m_render_targets[0]);
		}

	} /* internal */

	template<typename T>
	inline void AddRenderTargetReadBackTask(FrameGraph& frame_graph)
	{
		RenderTargetProperties rt_properties{
			false,
			std::nullopt,
			std::nullopt,
			ResourceState::COPY_DEST,
			ResourceState::COPY_SOURCE,
			false,
			Format::UNKNOWN,
			{ Format::R8G8B8A8_UNORM },
			1,
			true,
			true
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool) {
			internal::SetupReadBackTask<T>(rs, fg, handle);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle) {
			internal::ExecuteReadBackTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph&, RenderTaskHandle, bool) {
			// Nothing to destroy
		};
		desc.m_name = std::string(std::string("Render Target (") + std::string(typeid(T).name()) + std::string(") read-back task")).c_str();
		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COPY;
		desc.m_allow_multithreading = true;

		frame_graph.AddTask<RenderTargetReadBackTaskData>(desc);
	}

} /* wr */

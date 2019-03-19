#pragma once

#include "../frame_graph/frame_graph.hpp"
#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "../d3d12/d3d12_structs.hpp"

namespace wr
{
	struct DenoiserTaskData
	{
		RTcontext m_optix_context;

		RTbuffer m_denoiser_input;
		RTbuffer m_denoiser_output;

		RTpostprocessingstage m_denoiser_post_processor;
		RTvariable m_denoiser_post_processor_input;
		RTvariable m_denoiser_post_processor_output;

		RTcommandlist m_denoiser_command_list;

		d3d12::RenderTarget* m_prev_output;

		d3d12::ReadbackBufferResource* m_output_buffer;

		ID3D12Resource* m_upload_buffer;

		CommandList* m_download_cmd_list;
	};

	namespace internal
	{
		template<typename T>
		inline void SetupDenoiserTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<DeferredCompositionTaskData>(handle);
		}

		inline void ExecuteDenoiserTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& scene_graph, RenderTaskHandle handle)
		{

		}

		inline void DestroyDenoiserTask(FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{

		}

	} /* internal */

	template <typename T>
	inline void AddDenoiserTask(FrameGraph& frame_graph)
	{
		RenderTargetProperties rt_properties
		{
			RenderTargetProperties::IsRenderWindow(false),
			RenderTargetProperties::Width(std::nullopt),
			RenderTargetProperties::Height(std::nullopt),
			RenderTargetProperties::ExecuteResourceState(ResourceState::COPY_DEST),
			RenderTargetProperties::FinishedResourceState(ResourceState::COPY_SOURCE),
			RenderTargetProperties::CreateDSVBuffer(false),
			RenderTargetProperties::DSVFormat(Format::UNKNOWN),
			RenderTargetProperties::RTVFormats({ d3d12::settings::back_buffer_format }),
			RenderTargetProperties::NumRTVFormats(1),
			RenderTargetProperties::Clear(true),
			RenderTargetProperties::ClearDepth(false)
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::SetupDenoiserTask<T>(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle) {
			internal::ExecuteDenoiserTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::DestroyDenoiserTask(fg, handle, resize);
		};
		desc.m_name = "Post Processing";
		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COPY;
		desc.m_allow_multithreading = true;
		desc.m_post_processing = true;

		frame_graph.AddTask<PostProcessingData>(desc);
	}

} /* wr */

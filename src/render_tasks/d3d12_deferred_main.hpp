#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "../d3d12/d3d12_structured_buffer_pool.hpp"
#include "../d3d12/d3d12_model_pool.hpp"
#include "../d3d12/d3d12_resource_pool_texture.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../scene_graph/camera_node.hpp"
#include "../pipeline_registry.hpp"
#include "../engine_registry.hpp"

#include "../platform_independend_structs.hpp"
#include "d3d12_imgui_render_task.hpp"
#include "../scene_graph/camera_node.hpp"

namespace wr
{

	struct DeferredMainTaskData
	{
		d3d12::PipelineState* in_pipeline;
	};
	
	namespace internal
	{

		inline void SetupDeferredTask(RenderSystem&, FrameGraph& fg, RenderTaskHandle handle)
		{
			auto& data = fg.GetData<DeferredMainTaskData>(handle);

			auto& ps_registry = PipelineRegistry::Get();
			data.in_pipeline = (d3d12::PipelineState*)ps_registry.Find(pipelines::basic_deferred);
		}

		inline void ExecuteDeferredTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& scene_graph, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<DeferredMainTaskData>(handle);
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);

			if (n_render_system.m_render_window.has_value())
			{
				const auto viewport = n_render_system.m_viewport;
				const auto frame_idx = n_render_system.GetRenderWindow()->m_frame_idx;

				d3d12::BindViewport(cmd_list, viewport);
				d3d12::BindPipeline(cmd_list, data.in_pipeline);
				d3d12::SetPrimitiveTopology(cmd_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				auto d3d12_cb_handle = static_cast<D3D12ConstantBufferHandle*>(scene_graph.GetActiveCamera()->m_camera_cb);
				d3d12::BindConstantBuffer(cmd_list, d3d12_cb_handle->m_native, 0, frame_idx);

				scene_graph.Render(cmd_list, scene_graph.GetActiveCamera().get());
			}
		}

	} /* internal */

	inline void AddDeferredMainTask(FrameGraph& fg, std::optional<unsigned int> target_width, std::optional<unsigned int> target_height)
	{
		std::wstring name(L"Deferred Main");

		RenderTargetProperties rt_properties
		{
			RenderTargetProperties::IsRenderWindow(false),
			RenderTargetProperties::Width(target_width),
			RenderTargetProperties::Height(target_height),
			RenderTargetProperties::ExecuteResourceState(ResourceState::RENDER_TARGET),
			RenderTargetProperties::FinishedResourceState(ResourceState::NON_PIXEL_SHADER_RESOURCE),
			RenderTargetProperties::CreateDSVBuffer(true),
			RenderTargetProperties::DSVFormat(Format::D32_FLOAT),
			RenderTargetProperties::RTVFormats({ Format::R16G16B16A16_FLOAT, Format::R16G16B16A16_FLOAT, Format::R8G8B8A8_UNORM}),
			RenderTargetProperties::NumRTVFormats(3),
			RenderTargetProperties::Clear(true),
			RenderTargetProperties::ClearDepth(true),
			RenderTargetProperties::ResourceName(name)
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool) {
			internal::SetupDeferredTask(rs, fg, handle);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle) {
			internal::ExecuteDeferredTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph&, RenderTaskHandle, bool) {
			// Nothing to destroy
		};

		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::DIRECT;
		desc.m_allow_multithreading = true;

		fg.AddTask<DeferredMainTaskData>(desc);
	}

} /* wr */
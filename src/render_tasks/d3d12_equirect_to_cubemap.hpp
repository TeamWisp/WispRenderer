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

	struct EquirectToCubemapTaskData
	{
		D3D12Pipeline* in_pipeline;

		d3d12::TextureResource* in_equirectangular;
		d3d12::TextureResource* out_cubemap;
	};
	using EquirectToCubemapTask_t = RenderTask<EquirectToCubemapTaskData>;
	
	namespace internal
	{

		inline void SetupEquirectToCubemapTask(RenderSystem& render_system, EquirectToCubemapTask_t& task, EquirectToCubemapTaskData& data)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);

			auto& ps_registry = PipelineRegistry::Get();
			data.in_pipeline = (D3D12Pipeline*)ps_registry.Find(pipelines::basic_deferred); //TODO: pipelines::equirect_to_cubemap

			const auto cmd_list = task.GetCommandList<d3d12::CommandList>().first;
			auto render_target = task.GetRenderTarget<d3d12::RenderTarget>();
		}

		inline void ExecuteEquirectToCubemapTask(RenderSystem& render_system, EquirectToCubemapTask_t& task, SceneGraph& scene_graph, EquirectToCubemapTaskData& data)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);

			if (n_render_system.m_render_window.has_value())
			{
				auto cmd_list = task.GetCommandList<d3d12::CommandList>().first;

				d3d12::TextureResource* cubemap = data.out_cubemap;
				const auto viewport = d3d12::CreateViewport(cubemap->m_width, cubemap->m_height);

				const auto frame_idx = n_render_system.GetRenderWindow()->m_frame_idx;
				auto render_target = task.GetRenderTarget<d3d12::RenderTarget>();

				d3d12::BindViewport(cmd_list, viewport);
				d3d12::BindPipeline(cmd_list, data.in_pipeline->m_native);
				d3d12::SetPrimitiveTopology(cmd_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				struct ProjectionView_CB
				{
					DirectX::XMMATRIX m_view;
					DirectX::XMMATRIX m_projection;
				};

				auto d3d12_cb_handle = static_cast<D3D12ConstantBufferHandle*>(scene_graph.GetActiveCamera()->m_camera_cb);

				d3d12::BindConstantBuffer(cmd_list, d3d12_cb_handle->m_native, 0, frame_idx);

				scene_graph.Render(cmd_list, scene_graph.GetActiveCamera().get());
			}
		}

		inline void DestroyEquirectToCubemapTask(EquirectToCubemapTask_t& task, EquirectToCubemapTaskData& data)
		{
		}

	} /* internal */
	

	//! Used to create a new equirectangular to cubemap task.
	[[nodiscard]] inline std::unique_ptr<EquirectToCubemapTask_t> GetEquirectToCubemapTask(std::size_t width, std::size_t height)
	{
		auto ptr = std::make_unique<EquirectToCubemapTask_t>(nullptr, "Equirect To Cubemap Task", RenderTaskType::DIRECT, true,
			RenderTargetProperties{
				false,
				width,
				height,
				ResourceState::RENDER_TARGET,
				ResourceState::NON_PIXEL_SHADER_RESOURCE,
				true,
				Format::D32_FLOAT,
				{ Format::R32G32B32A32_FLOAT },
				1,
				true,
				true
			},
			[](RenderSystem& render_system, EquirectToCubemapTask_t& task, EquirectToCubemapTaskData& data, bool) { internal::SetupEquirectToCubemapTask(render_system, task, data); },
			[](RenderSystem& render_system, EquirectToCubemapTask_t& task, SceneGraph& scene_graph, EquirectToCubemapTaskData& data) { internal::ExecuteEquirectToCubemapTask(render_system, task, scene_graph, data); },
			[](EquirectToCubemapTask_t& task, EquirectToCubemapTaskData& data, bool) { internal::DestroyEquirectToCubemapTask(task, data); }
		);

		return ptr;
	}

} /* wr */
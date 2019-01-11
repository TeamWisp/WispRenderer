#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "../d3d12/d3d12_structured_buffer_pool.hpp"
#include "../d3d12/d3d12_model_pool.hpp"
#include "../d3d12/d3d12_resource_pool_texture.hpp"
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

		std::shared_ptr<ConstantBufferPool> camera_cb_pool;
		DirectX::XMMATRIX proj_mat;
		DirectX::XMMATRIX view_mat;

		bool should_run = false;
	};

	namespace internal
	{

		inline void SetupEquirectToCubemapTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, TextureHandle* in_equirect, TextureHandle* out_cubemap)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<EquirectToCubemapTaskData>(handle);

			auto& ps_registry = PipelineRegistry::Get();
			data.in_pipeline = (D3D12Pipeline*)ps_registry.Find(pipelines::equirect_to_cubemap);

			d3d12::TextureResource* equirect_text = static_cast<d3d12::TextureResource*>(in_equirect->m_pool->GetTexture(in_equirect->m_id));
			d3d12::TextureResource* cubemap_text = static_cast<d3d12::TextureResource*>(out_cubemap->m_pool->GetTexture(out_cubemap->m_id));

			data.camera_cb_pool = rs.CreateConstantBufferPool(1);

			data.proj_mat = DirectX::XMMatrixPerspectiveFovRH(DirectX::XMConvertToRadians(90.0f), 1.0f, 0.1f, 10.0f);

			data.view_mat = DirectX::XMMatrixLookAtRH(DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
				DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f),
				DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

			/*data.views_array[1] = DirectX::XMMatrixLookAtRH(DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
															DirectX::XMVectorSet(-1.0f, 0.0f, 0.0f, 1.0f),
															DirectX::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f));

			data.views_array[2] = DirectX::XMMatrixLookAtRH(DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
															DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f),
															DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f));

			data.views_array[3] = DirectX::XMMatrixLookAtRH(DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
															DirectX::XMVectorSet(0.0f, -1.0f, 0.0f, 1.0f),
															DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f));

			data.views_array[4] = DirectX::XMMatrixLookAtRH(DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
															DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f),
															DirectX::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f));

			data.views_array[5] = DirectX::XMMatrixLookAtRH(DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
															DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 1.0f),
															DirectX::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f));*/
		}

		inline void ExecuteEquirectToCubemapTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& scene_graph, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<EquirectToCubemapTaskData>(handle);

			if (n_render_system.m_render_window.has_value())
			{
				auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
				auto render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);

				d3d12::TextureResource* cubemap = data.out_cubemap;

				const auto viewport = d3d12::CreateViewport(cubemap->m_width, cubemap->m_height);

				const auto frame_idx = n_render_system.GetRenderWindow()->m_frame_idx;

				d3d12::BindViewport(cmd_list, viewport);
				d3d12::BindPipeline(cmd_list, data.in_pipeline->m_native);
				d3d12::SetPrimitiveTopology(cmd_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				struct ProjectionView_CB
				{
					DirectX::XMMATRIX m_view;
					DirectX::XMMATRIX m_projection;
				};

				auto d3d12_cb_handle = static_cast<D3D12ConstantBufferHandle*>(data.camera_cb_pool->Create(sizeof(ProjectionView_CB)));

				ProjectionView_CB cb_data;
				cb_data.m_projection = data.proj_mat;
				cb_data.m_view = data.view_mat;

				d3d12_cb_handle->m_pool->Update(d3d12_cb_handle, sizeof(ProjectionView_CB), 0, (uint8_t*)&cb_data);

				d3d12::BindConstantBuffer(cmd_list, d3d12_cb_handle->m_native, 1, frame_idx);
				d3d12::SetShaderTexture(cmd_list, 2, 0, data.in_equirectangular);

				for (uint32_t i = 0; i < 6; ++i)
				{
					//Get render target handle.
					d3d12::DescHeapCPUHandle rtv_handle = data.in_equirectangular->m_rtv_allocation->GetDescriptorHandle(i);

					cmd_list->m_native->OMSetRenderTargets(1, &rtv_handle.m_native, false, nullptr);

					const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

					cmd_list->m_native->ClearRenderTargetView(rtv_handle.m_native, clear_color, 0, nullptr);

					d3d12::BindCompute32BitConstants(cmd_list, &i, 1, 0, 0);

					//bind cube and render
					Model* cube_model = rs.GetSimpleShape(RenderSystem::SimpleShapes::CUBE);

					//Render meshes
					for (auto& mesh : cube_model->m_meshes)
					{
						auto n_mesh = static_cast<D3D12ModelPool*>(cube_model->m_model_pool)->GetMeshData(mesh.first->id);
					
						d3d12::BindVertexBuffer(cmd_list, static_cast<D3D12ModelPool*>(cube_model->m_model_pool)->GetVertexStagingBuffer(),
								0, static_cast<D3D12ModelPool*>(cube_model->m_model_pool)->GetVertexStagingBuffer()->m_size,
								n_mesh->m_vertex_staging_buffer_stride);

						d3d12::BindIndexBuffer(cmd_list, static_cast<D3D12ModelPool*>(cube_model->m_model_pool)->GetIndexStagingBuffer(),
								0, static_cast<D3D12ModelPool*>(cube_model->m_model_pool)->GetIndexStagingBuffer()->m_size);


						d3d12::BindDescriptorHeaps(cmd_list, frame_idx);

						if (n_mesh->m_index_count != 0)
						{
							d3d12::DrawIndexed(cmd_list, n_mesh->m_index_count, 1, n_mesh->m_index_staging_buffer_offset, n_mesh->m_vertex_staging_buffer_offset);
						}
						else
						{
							d3d12::Draw(cmd_list, n_mesh->m_vertex_count, 1, n_mesh->m_vertex_staging_buffer_offset);
						}
					}
				}

			}
		}
	} /* internal */

	inline void AddEquirectToCubemapTask(FrameGraph& fg, TextureHandle* in_equirect, TextureHandle* out_cubemap)
	{
		RenderTargetProperties rt_properties
		{
			false,
			std::nullopt,
			std::nullopt,
			ResourceState::RENDER_TARGET,
			ResourceState::NON_PIXEL_SHADER_RESOURCE,
			true,
			Format::D32_FLOAT,
			{ Format::R32G32B32A32_FLOAT },
			1,
			true,
			true
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [&](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool) {
			internal::SetupEquirectToCubemapTask(rs, fg, handle, in_equirect, out_cubemap);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle) {
			internal::ExecuteEquirectToCubemapTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph&, RenderTaskHandle, bool) {
			// Nothing to destroy
		};
		desc.m_name = "Equirect To Cubemap";
		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::DIRECT;
		desc.m_allow_multithreading = true;

		fg.AddTask<EquirectToCubemapTaskData>(desc);
	}

} /* wr */
#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_defines.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "../d3d12/d3d12_structured_buffer_pool.hpp"
#include "../d3d12/d3d12_model_pool.hpp"
#include "../d3d12/d3d12_resource_pool_texture.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../scene_graph/camera_node.hpp"
#include "../scene_graph/skybox_node.hpp"
#include "../d3d12/d3d12_pipeline_registry.hpp"
#include "../engine_registry.hpp"

#include "../platform_independend_structs.hpp"
#include "d3d12_imgui_render_task.hpp"
#include "d3d12_equirect_to_cubemap.hpp"

namespace wr
{
	struct CubemapConvolutionTaskData
	{
		D3D12Pipeline* in_pipeline;

		TextureHandle in_radiance;
		TextureHandle out_irradiance;
		TextureHandle out_pref_env_map;

		std::shared_ptr<ConstantBufferPool> camera_cb_pool;
		D3D12ConstantBufferHandle* cb_handle;

		DirectX::XMMATRIX proj_mat;
		DirectX::XMMATRIX view_mat[6];

		bool should_run = true;
	};

	namespace internal
	{
		struct ProjectionView_CBuffer
		{
			DirectX::XMMATRIX m_projection;
			DirectX::XMMATRIX m_view[6];
		};

		inline void SetupCubemapConvolutionTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			if (resize)
			{
				return;
			}

			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<CubemapConvolutionTaskData>(handle);

			auto& ps_registry = PipelineRegistry::Get();
			data.in_pipeline = (D3D12Pipeline*)ps_registry.Find(pipelines::cubemap_convolution);

			data.camera_cb_pool = rs.CreateConstantBufferPool(2_mb);
			data.cb_handle = static_cast<D3D12ConstantBufferHandle*>(data.camera_cb_pool->Create(sizeof(ProjectionView_CBuffer)));

			data.proj_mat = DirectX::XMMatrixPerspectiveFovRH(DirectX::XMConvertToRadians(90.0f), 1.0f, 0.1f, 10.0f);

			data.view_mat[0] = DirectX::XMMatrixLookAtRH(DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
				DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f),
				DirectX::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f));

			data.view_mat[1] = DirectX::XMMatrixLookAtRH(DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
				DirectX::XMVectorSet(-1.0f, 0.0f, 0.0f, 1.0f),
				DirectX::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f));

			data.view_mat[2] = DirectX::XMMatrixLookAtRH(DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
				DirectX::XMVectorSet(0.0f, -1.0f, 0.0f, 1.0f),
				DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f));

			data.view_mat[3] = DirectX::XMMatrixLookAtRH(DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
				DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f),
				DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f));

			data.view_mat[4] = DirectX::XMMatrixLookAtRH(DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
				DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f),
				DirectX::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f));

			data.view_mat[5] = DirectX::XMMatrixLookAtRH(DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
				DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 1.0f),
				DirectX::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f));
		}

		inline void ExecuteCubemapConvolutionTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& scene_graph, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<CubemapConvolutionTaskData>(handle);

			auto& pred_data = fg.GetPredecessorData<EquirectToCubemapTaskData>();

			auto skybox_node = scene_graph.GetCurrentSkybox();

			//Does it need convolution? And does it have a cubemap already?
			if (skybox_node->m_irradiance != std::nullopt && skybox_node->m_skybox != std::nullopt)
			{
				return;
			}

			data.in_radiance = pred_data.out_cubemap;
			data.out_pref_env_map = pred_data.out_pref_env;

			skybox_node->m_irradiance = skybox_node->m_skybox.value().m_pool->CreateCubemap("ConvolutedMap", 128, 128, 1, wr::Format::R32G32B32A32_FLOAT, true);;

			data.out_irradiance = skybox_node->m_irradiance.value();

			d3d12::TextureResource* radiance = static_cast<d3d12::TextureResource*>(data.in_radiance.m_pool->GetTextureResource(data.in_radiance));
			d3d12::TextureResource* irradiance = static_cast<d3d12::TextureResource*>(data.out_irradiance.m_pool->GetTextureResource(data.out_irradiance));

			if (data.should_run && radiance->m_is_staged)
			{
				if (n_render_system.m_render_window.has_value())
				{
					auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
					auto render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);

					const auto viewport = d3d12::CreateViewport(irradiance->m_width, irradiance->m_height);

					const auto frame_idx = n_render_system.GetRenderWindow()->m_frame_idx;

					d3d12::BindViewport(cmd_list, viewport);
					d3d12::BindPipeline(cmd_list, data.in_pipeline->m_native);
					d3d12::SetPrimitiveTopology(cmd_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

					ProjectionView_CBuffer cb_data;
					cb_data.m_view[0] = data.view_mat[0];
					cb_data.m_view[1] = data.view_mat[1];
					cb_data.m_view[2] = data.view_mat[2];
					cb_data.m_view[3] = data.view_mat[3];
					cb_data.m_view[4] = data.view_mat[4];
					cb_data.m_view[5] = data.view_mat[5];
					cb_data.m_projection = data.proj_mat;

					data.cb_handle->m_pool->Update(data.cb_handle, sizeof(ProjectionView_CBuffer), 0, frame_idx, (uint8_t*)&cb_data);

					d3d12::BindConstantBuffer(cmd_list, data.cb_handle->m_native, 1, frame_idx);

					for (uint32_t i = 0; i < 6; ++i)
					{
						//Get render target handle.
						d3d12::DescHeapCPUHandle rtv_handle = irradiance->m_rtv_allocation->GetDescriptorHandle(i);

						cmd_list->m_native->OMSetRenderTargets(1, &rtv_handle.m_native, false, nullptr);

						d3d12::Bind32BitConstants(cmd_list, &i, 1, 0, 0);

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

							constexpr unsigned int env_idx = rs_layout::GetHeapLoc(params::cubemap_convolution, params::CubemapConvolutionE::ENVIRONMENT_CUBEMAP);
							d3d12::SetShaderSRV(cmd_list, 2, env_idx, radiance);
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

					d3d12::Transition(cmd_list, irradiance, irradiance->m_subresource_states[0], ResourceState::PIXEL_SHADER_RESOURCE);

					data.should_run = false;
				}
			}
			//if (data.should_run)
		}

		inline void DestroyCubemapConvolutionTask(FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			if (resize)
			{
				return;
			}
		}

	} /* internal */

	inline void AddCubemapConvolutionTask(FrameGraph& fg)
	{
		std::wstring name(L"Cubemap Convolution");

		RenderTargetProperties rt_properties
		{
			RenderTargetProperties::IsRenderWindow(false),
			RenderTargetProperties::Width(std::nullopt),
			RenderTargetProperties::Height(std::nullopt),
			RenderTargetProperties::ExecuteResourceState(ResourceState::RENDER_TARGET),
			RenderTargetProperties::FinishedResourceState(ResourceState::NON_PIXEL_SHADER_RESOURCE),
			RenderTargetProperties::CreateDSVBuffer(true),
			RenderTargetProperties::DSVFormat(Format::D32_FLOAT),
			RenderTargetProperties::RTVFormats({ Format::R32G32B32A32_FLOAT }),
			RenderTargetProperties::NumRTVFormats(1),
			RenderTargetProperties::Clear(true),
			RenderTargetProperties::ClearDepth(true),
			RenderTargetProperties::ResourceName(name)
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [&](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::SetupCubemapConvolutionTask(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle) {
			internal::ExecuteCubemapConvolutionTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::DestroyCubemapConvolutionTask(fg, handle, resize);
		};

		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::DIRECT;
		desc.m_allow_multithreading = true;

		fg.AddTask<CubemapConvolutionTaskData>(desc);
	}

} /* wr */


/*
OLD COMPUTE CODE, KEEPING IT HERE FOR POSSIBLE FUTURE USAGE.
*/
/*
			struct CubemapConvolutionTaskData
	{
		D3D12Pipeline* in_pipeline;

		d3d12::TextureResource* in_radiance; //The skybox hdr cubemap
		d3d12::TextureResource* out_irradiance;

		bool should_run = true;
	};

	namespace internal
	{
		inline void SetupCubemapConvolutionTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, TextureHandle in_radiance, TextureHandle out_irradiance)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<CubemapConvolutionTaskData>(handle);

			auto& ps_registry = PipelineRegistry::Get();
			data.in_pipeline = (D3D12Pipeline*)ps_registry.Find(pipelines::cubemap_convolution);

			data.in_radiance = static_cast<d3d12::TextureResource*>(in_radiance.m_pool->GetTexture(in_radiance.m_id));
			data.out_irradiance = static_cast<d3d12::TextureResource*>(out_irradiance.m_pool->GetTexture(out_irradiance.m_id));

			D3D12TexturePool* pool = static_cast<D3D12TexturePool*>(out_irradiance.m_pool);

			d3d12::DescHeapCPUHandle uav_handle = data.out_irradiance->m_uav_allocation.GetDescriptorHandle();

			d3d12::CreateUAVFromTexture(data.out_irradiance, uav_handle, 0);
		}

		inline void ExecuteCubemapConvolutionTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& scene_graph, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<CubemapConvolutionTaskData>(handle);

			if (data.should_run)
			{
				if (n_render_system.m_render_window.has_value())
				{
					auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
					const auto frame_idx = n_render_system.GetFrameIdx();

					d3d12::BindComputePipeline(cmd_list, data.in_pipeline->m_native);

					//Transition and bind SRV
					d3d12::Transition(cmd_list, data.in_radiance, data.in_radiance->m_current_state, ResourceState::PIXEL_SHADER_RESOURCE);
					d3d12::SetShaderTexture(cmd_list, 0, 0, data.in_radiance);

					//Transition and bind UAV
					d3d12::Transition(cmd_list, data.out_irradiance, data.out_irradiance->m_current_state, ResourceState::UNORDERED_ACCESS);
					d3d12::SetShaderUAV(cmd_list, 0, 1, data.out_irradiance);

					d3d12::BindDescriptorHeaps(cmd_list, frame_idx);

					d3d12::Dispatch(cmd_list, data.out_irradiance->m_width/32, data.out_irradiance->m_height/32, 6);

					d3d12::Transition(cmd_list, data.out_irradiance, data.out_irradiance->m_current_state, ResourceState::PIXEL_SHADER_RESOURCE);

					data.should_run = false;
				}
			}
		}

	}
	*/
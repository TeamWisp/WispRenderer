#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_defines.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "../d3d12/d3d12_structured_buffer_pool.hpp"
#include "../d3d12/d3d12_resource_pool_texture.hpp"
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

		TextureHandle in_equirect;
		TextureHandle out_cubemap;

		std::shared_ptr<ConstantBufferPool> camera_cb_pool;
		D3D12ConstantBufferHandle* cb_handle;

		DirectX::XMMATRIX proj_mat;
		DirectX::XMMATRIX view_mat[6];

		bool should_run = true;
	};

	namespace internal
	{
		struct ProjectionView_CB
		{
			DirectX::XMMATRIX m_projection;
			DirectX::XMMATRIX m_view[6];
		};

		struct PrefilterEnv_CB
		{
			DirectX::XMFLOAT2 texture_size;
			float	 roughness;
			uint32_t cubemap_face;
		};

		inline void PrefilterCubemapFace(d3d12::TextureResource* texture, CommandList* cmd_list, DescriptorAllocator* alloc, unsigned int array_slice)
		{
			wr::d3d12::CommandList* d3d12_cmd_list = static_cast<wr::d3d12::CommandList*>(cmd_list);

			//Create shader resource view for the source texture in the descriptor heap
			DescriptorAllocation srv_alloc = alloc->Allocate();
			d3d12::DescHeapCPUHandle srv_handle = srv_alloc.GetDescriptorHandle();

			d3d12::CreateSRVFromTexture(texture, srv_handle);

			PrefilterEnv_CB prefilter_env_cb;

			for (uint32_t src_mip = 0; src_mip < texture->m_mip_levels - 1u; ++src_mip)
			{
				uint32_t src_width = texture->m_width >> src_mip;
				uint32_t src_height = texture->m_height >> src_mip;
				uint32_t dst_width = src_width >> 1;
				uint32_t dst_height = src_height >> 1;

				prefilter_env_cb.texture_size = DirectX::XMFLOAT2((float)dst_width, (float)dst_height);
				prefilter_env_cb.roughness = (float)(src_mip + 1u) / (float)(texture->m_mip_levels - 1);
				prefilter_env_cb.cubemap_face = array_slice;

				//Create views
				//d3d12::CreateSRVFromCubemapFace(texture, srv_handle, 1, src_mip, array_slice);

				DescriptorAllocation uav_alloc = alloc->Allocate();
				d3d12::DescHeapCPUHandle uav_handle = uav_alloc.GetDescriptorHandle();

				d3d12::CreateUAVFromCubemapFace(texture, uav_handle, src_mip + 1, array_slice);

				//Set shader variables
				d3d12::BindCompute32BitConstants(d3d12_cmd_list, &prefilter_env_cb, sizeof(PrefilterEnv_CB) / sizeof(uint32_t), 0, 0);

				d3d12::Transition(d3d12_cmd_list, texture, texture->m_subresource_states[src_mip], ResourceState::PIXEL_SHADER_RESOURCE, src_mip, 1);
				d3d12::SetShaderSRV(d3d12_cmd_list, 1, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::mip_mapping, params::MipMappingE::SOURCE)), srv_handle);

				d3d12::Transition(d3d12_cmd_list, texture, texture->m_subresource_states[src_mip + 1], ResourceState::UNORDERED_ACCESS, src_mip + 1, 1);
				d3d12::SetShaderUAV(d3d12_cmd_list, 1, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::mip_mapping, params::MipMappingE::DEST)), uav_handle);

				d3d12::Dispatch(d3d12_cmd_list, ((dst_width + 8 - 1) / 8), ((dst_height + 8 - 1) / 8), 1u);

				//Wait for all accesses to the destination texture UAV to be finished before generating the next mipmap, as it will be the source texture for the next mipmap
				d3d12::UAVBarrier(d3d12_cmd_list, texture, 1);

				d3d12::Transition(d3d12_cmd_list, texture, texture->m_subresource_states[src_mip + 1], ResourceState::PIXEL_SHADER_RESOURCE, src_mip + 1, 1);
			}

			texture->m_need_mips = false;
		}


		inline void PrefilterCubemap(d3d12::CommandList* cmd_list, wr::TextureHandle texture)
		{
			D3D12Pipeline* pipeline = static_cast<D3D12Pipeline*>(PipelineRegistry::Get().Find(pipelines::cubemap_prefiltering));

			d3d12::BindComputePipeline(cmd_list, pipeline->m_native);
			
			//Get allocator from pool
			auto allocator = static_cast<D3D12TexturePool*>(texture.m_pool)->GetMipmappingAllocator();

			d3d12::TextureResource* cubemap_text = static_cast<d3d12::TextureResource*>(texture.m_pool->GetTexture(texture.m_id));

			for (uint32_t i = 0; i < 6; ++i)
			{
				PrefilterCubemapFace(cubemap_text, cmd_list, allocator, i);
			}
		}


		inline void SetupEquirectToCubemapTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<EquirectToCubemapTaskData>(handle);

			auto& ps_registry = PipelineRegistry::Get();
			data.in_pipeline = (D3D12Pipeline*)ps_registry.Find(pipelines::equirect_to_cubemap);

			data.camera_cb_pool = rs.CreateConstantBufferPool(2_mb);
			data.cb_handle = static_cast<D3D12ConstantBufferHandle*>(data.camera_cb_pool->Create(sizeof(ProjectionView_CB)));

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

		inline void ExecuteEquirectToCubemapTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& scene_graph, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<EquirectToCubemapTaskData>(handle);

			auto skybox_node = scene_graph.GetCurrentSkybox();

			//Does it need conversion?
			if (skybox_node->m_skybox != std::nullopt)
			{
				return;
			}

			data.in_equirect = skybox_node->m_hdr;
			
			skybox_node->m_skybox = skybox_node->m_hdr.m_pool->CreateCubemap("EnvironmentMap", 128, 128, 5, wr::Format::R32G32B32A32_FLOAT, true);

			data.out_cubemap = skybox_node->m_skybox.value();

			d3d12::TextureResource* equirect_text = static_cast<d3d12::TextureResource*>(data.in_equirect.m_pool->GetTexture(data.in_equirect.m_id));
			d3d12::TextureResource* cubemap_text = static_cast<d3d12::TextureResource*>(data.out_cubemap.m_pool->GetTexture(data.out_cubemap.m_id));

			if (n_render_system.m_render_window.has_value())
			{
				auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
				auto render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);

				const auto viewport = d3d12::CreateViewport(cubemap_text->m_width, cubemap_text->m_height);

				const auto frame_idx = n_render_system.GetRenderWindow()->m_frame_idx;

				d3d12::BindViewport(cmd_list, viewport);
				d3d12::BindPipeline(cmd_list, data.in_pipeline->m_native);
				d3d12::SetPrimitiveTopology(cmd_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				ProjectionView_CB cb_data;
				cb_data.m_view[0] = data.view_mat[0];
				cb_data.m_view[1] = data.view_mat[1];
				cb_data.m_view[2] = data.view_mat[2];
				cb_data.m_view[3] = data.view_mat[3];
				cb_data.m_view[4] = data.view_mat[4];
				cb_data.m_view[5] = data.view_mat[5];
				cb_data.m_projection = data.proj_mat;

				data.cb_handle->m_pool->Update(data.cb_handle, sizeof(ProjectionView_CB), 0, frame_idx, (uint8_t*)&cb_data);

				d3d12::BindConstantBuffer(cmd_list, data.cb_handle->m_native, 1, frame_idx);

				for (uint32_t i = 0; i < 6; ++i)
				{
					//Get render target handle.
					d3d12::DescHeapCPUHandle rtv_handle = cubemap_text->m_rtv_allocation->GetDescriptorHandle(i);

					cmd_list->m_native->OMSetRenderTargets(1, &rtv_handle.m_native, false, nullptr);

					d3d12::Bind32BitConstants(cmd_list, &i, 1, 0, 0);

					//bind cube and render
					Model* cube_model = rs.GetSimpleShape(RenderSystem::SimpleShapes::CUBE);

					//Render meshes
					for (auto& mesh : cube_model->m_meshes)
					{
						wr::D3D12ModelPool* pool = static_cast<D3D12ModelPool*>(cube_model->m_model_pool);

						auto n_mesh = pool->GetMeshData(mesh.first->id);

						d3d12::BindVertexBuffer(cmd_list, pool->GetVertexStagingBuffer(), 0, pool->GetVertexStagingBuffer()->m_size, n_mesh->m_vertex_staging_buffer_stride);

						d3d12::BindIndexBuffer(cmd_list, pool->GetIndexStagingBuffer(), 0, pool->GetIndexStagingBuffer()->m_size);

						constexpr unsigned int srv_idx = rs_layout::GetHeapLoc(params::cubemap_conversion, params::CubemapConversionE::EQUIRECTANGULAR_TEXTURE);
						d3d12::SetShaderSRV(cmd_list, 2, srv_idx, equirect_text);

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

				d3d12::Transition(cmd_list, cubemap_text, cubemap_text->m_subresource_states[0], ResourceState::PIXEL_SHADER_RESOURCE);


				//TEST MIPMAPPING
				//D3D12Pipeline* pipeline = static_cast<D3D12Pipeline*>(PipelineRegistry::Get().Find(pipelines::mip_mapping));

				//d3d12::BindComputePipeline(cmd_list, pipeline->m_native);

				//auto texture_pool = std::static_pointer_cast<D3D12TexturePool>(n_render_system.m_texture_pools[0]);

				//texture_pool->GenerateMips_Cubemap(cubemap_text, cmd_list, 0);
				//texture_pool->GenerateMips_Cubemap(cubemap_text, cmd_list, 1);
				//texture_pool->GenerateMips_Cubemap(cubemap_text, cmd_list, 2);
				//texture_pool->GenerateMips_Cubemap(cubemap_text, cmd_list, 3);
				//texture_pool->GenerateMips_Cubemap(cubemap_text, cmd_list, 4);
				//texture_pool->GenerateMips_Cubemap(cubemap_text, cmd_list, 5);

				PrefilterCubemap(cmd_list, data.out_cubemap);
			}
		}
	} /* internal */

	inline void AddEquirectToCubemapTask(FrameGraph& fg)
	{
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
			RenderTargetProperties::ClearDepth(true)
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [&](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::SetupEquirectToCubemapTask(rs, fg, handle, resize);
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
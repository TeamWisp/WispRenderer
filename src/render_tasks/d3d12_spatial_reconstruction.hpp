#pragma once

#include "../frame_graph/frame_graph.hpp"
#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "../render_tasks/d3d12_rt_hybrid_task.hpp"
#include "../render_tasks/d3d12_deferred_main.hpp"

namespace wr
{
	struct SpatialReconstructionData
	{
		d3d12::PipelineState* pipeline;
		D3D12ConstantBufferHandle* camera_cb;
		DescriptorAllocator* allocator;
		DescriptorAllocation output;
		DescriptorAllocation reflection_buffer;
		DescriptorAllocation gbuffer_normal;
		DescriptorAllocation gbuffer_roughness;
		DescriptorAllocation gbuffer_depth;
	};

	namespace temp
	{

		struct SpatialReconstructionCameraData
		{
			DirectX::XMMATRIX inv_projection;
			DirectX::XMMATRIX inv_view;
			DirectX::XMMATRIX view;

			DirectX::XMFLOAT2 padding;
			float near_plane, far_plane;
		};

	}

	namespace internal
	{

		inline void SetupSpatialReconstructionTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<SpatialReconstructionData>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);
			auto hybrid_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<RTHybridData>());
			auto gbuffer_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<DeferredMainTaskData>());

			if (!resize)
			{
				data.allocator = new DescriptorAllocator(n_render_system, wr::DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV);
				data.output = data.allocator->Allocate();
				data.reflection_buffer = data.allocator->Allocate(2);
				data.gbuffer_normal = data.allocator->Allocate(d3d12::settings::num_back_buffers);
				data.gbuffer_roughness = data.allocator->Allocate(d3d12::settings::num_back_buffers);
				data.gbuffer_depth = data.allocator->Allocate();

				auto& ps_registry = PipelineRegistry::Get();
				data.pipeline = ((D3D12Pipeline*)ps_registry.Find(pipelines::spatial_reconstruction))->m_native;

				data.camera_cb = static_cast<D3D12ConstantBufferHandle*>(n_render_system.m_camera_pool->Create(sizeof(temp::SpatialReconstructionCameraData)));
			}

			// Deferred data

			for (uint32_t i = 0; i < d3d12::settings::num_back_buffers; ++i)
			{
				auto albedo_handle = data.gbuffer_roughness.GetDescriptorHandle(i);
				auto normal_handle = data.gbuffer_normal.GetDescriptorHandle(i);

				d3d12::CreateSRVFromSpecificRTV(gbuffer_rt, albedo_handle, 0, gbuffer_rt->m_create_info.m_rtv_formats[0]);
				d3d12::CreateSRVFromSpecificRTV(gbuffer_rt, normal_handle, 1, gbuffer_rt->m_create_info.m_rtv_formats[1]);

			}

			d3d12::CreateSRVFromDSV(gbuffer_rt, 
				data.gbuffer_depth.GetDescriptorHandle());


			// Filtered output
			{
				auto cpu_handle = data.output.GetDescriptorHandle();
				d3d12::CreateUAVFromSpecificRTV(n_render_target, cpu_handle, 0, n_render_target->m_create_info.m_rtv_formats[0]);
			}

			// Reflection buffer input
			{
				auto cpu_handle = data.reflection_buffer.GetDescriptorHandle();
				d3d12::CreateSRVFromSpecificRTV(hybrid_rt, cpu_handle, 0, hybrid_rt->m_create_info.m_rtv_formats[0]);

				cpu_handle = data.reflection_buffer.GetDescriptorHandle(1);
				d3d12::CreateSRVFromSpecificRTV(hybrid_rt, cpu_handle, 2, hybrid_rt->m_create_info.m_rtv_formats[2]);
			}

		}

		inline void ExecuteSpatialReconstructionTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& n_device = n_render_system.m_device->m_native;
			auto& data = fg.GetData<SpatialReconstructionData>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);
			auto frame_idx = n_render_system.GetFrameIdx();
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
			const auto viewport = n_render_system.m_viewport;

			fg.WaitForPredecessorTask<DeferredMainTaskData>();
			fg.WaitForPredecessorTask<RTHybridData>();

			d3d12::BindComputePipeline(cmd_list, data.pipeline);

			// Update camera constant buffer
			auto camera = sg.GetActiveCamera();
			temp::SpatialReconstructionCameraData cam_data;
			cam_data.inv_view = DirectX::XMMatrixInverse(nullptr, camera->m_view);
			cam_data.inv_projection = DirectX::XMMatrixInverse(nullptr, camera->m_projection);
			cam_data.view = camera->m_view;
			cam_data.near_plane = camera->m_frustum_near;
			cam_data.far_plane = camera->m_frustum_far;
			n_render_system.m_camera_pool->Update(data.camera_cb, sizeof(temp::SpatialReconstructionCameraData), 0, frame_idx, (std::uint8_t*)&cam_data);

			d3d12::BindComputeConstantBuffer(cmd_list, data.camera_cb->m_native, 1, frame_idx);

			{
				constexpr unsigned int dest_idx = rs_layout::GetHeapLoc(params::spatial_reconstruction, params::SpatialReconstructionE::OUTPUT);
				auto handle_uav = data.output.GetDescriptorHandle();
				d3d12::SetShaderUAV(cmd_list, 0, dest_idx, handle_uav);
			}

			{
				constexpr unsigned int dest_idx = rs_layout::GetHeapLoc(params::spatial_reconstruction, params::SpatialReconstructionE::REFLECTION_BUFFER);
				auto handle_srv = data.reflection_buffer.GetDescriptorHandle();
				d3d12::SetShaderSRV(cmd_list, 0, dest_idx, handle_srv);
				handle_srv = data.reflection_buffer.GetDescriptorHandle(1);
				d3d12::SetShaderSRV(cmd_list, 0, dest_idx + 1, handle_srv);
			}

			{
				constexpr unsigned int dest_idx = rs_layout::GetHeapLoc(params::spatial_reconstruction, params::SpatialReconstructionE::GBUFFERS); 

				d3d12::DescHeapCPUHandle albedo_handle = data.gbuffer_roughness.GetDescriptorHandle(frame_idx);
				d3d12::SetShaderSRV(cmd_list, 0, dest_idx, albedo_handle);
				
				d3d12::DescHeapCPUHandle normal_handle = data.gbuffer_normal.GetDescriptorHandle(frame_idx);
				d3d12::SetShaderSRV(cmd_list, 0, dest_idx + 1, normal_handle);
				
				d3d12::DescHeapCPUHandle depth_handle = data.gbuffer_depth.GetDescriptorHandle();
				d3d12::SetShaderSRV(cmd_list, 0, dest_idx + 2, depth_handle);
			}

			d3d12::Dispatch(cmd_list,
				uint32_t(std::ceil(viewport.m_viewport.Width / 16.f)),
				uint32_t(std::ceil(viewport.m_viewport.Height / 16.f)),
				1);
		}

	} /* internal */

	inline void AddSpatialReconstructionTask(FrameGraph& frame_graph)
	{
		RenderTargetProperties rt_properties
		{
			RenderTargetProperties::IsRenderWindow(false),
			RenderTargetProperties::Width(std::nullopt),
			RenderTargetProperties::Height(std::nullopt),
			RenderTargetProperties::ExecuteResourceState(ResourceState::UNORDERED_ACCESS),
			RenderTargetProperties::FinishedResourceState(ResourceState::COPY_SOURCE),
			RenderTargetProperties::CreateDSVBuffer(false),
			RenderTargetProperties::DSVFormat(Format::UNKNOWN),
			RenderTargetProperties::RTVFormats({ wr::Format::R16G16B16A16_FLOAT }),
			RenderTargetProperties::NumRTVFormats(1),
			RenderTargetProperties::Clear(false),
			RenderTargetProperties::ClearDepth(false),
			RenderTargetProperties::ResourceName(L"Spatial reconstruction"),
			RenderTargetProperties::ResolutionScalar(1.f)
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [](RenderSystem & rs, FrameGraph & fg, RenderTaskHandle handle, bool resize) {
			internal::SetupSpatialReconstructionTask(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](RenderSystem & rs, FrameGraph & fg, SceneGraph & sg, RenderTaskHandle handle) {
			internal::ExecuteSpatialReconstructionTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph & fg, RenderTaskHandle handle, bool resize) { };
		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		frame_graph.AddTask<SpatialReconstructionData>(desc, fg_dep<DeferredMainTaskData, RTHybridData>());
	}

} /* wr */

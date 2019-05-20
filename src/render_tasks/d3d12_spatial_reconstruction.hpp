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
		DescriptorAllocation allocation;
	};

	namespace temp {

		struct SpatialReconstructionCameraData {
			DirectX::XMMATRIX inv_projection;
			DirectX::XMMATRIX inv_view;
			DirectX::XMMATRIX view;
		};

	}

	namespace internal
	{

		inline void SetupSpatialReconstructionTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			if (!resize)
			{
				auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
				auto& data = fg.GetData<SpatialReconstructionData>(handle);
				auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);

				data.allocator = new DescriptorAllocator(n_render_system, wr::DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV);
				data.allocation = data.allocator->Allocate(6);

				auto& ps_registry = PipelineRegistry::Get();
				data.pipeline = ((D3D12Pipeline*)ps_registry.Find(pipelines::spatial_reconstruction))->m_native;

				data.camera_cb = static_cast<D3D12ConstantBufferHandle*>(n_render_system.m_camera_pool->Create(sizeof(temp::SpatialReconstructionCameraData)));
			}

		}

		inline void ExecuteSpatialReconstructionTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<SpatialReconstructionData>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);
			auto frame_idx = n_render_system.GetFrameIdx();
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
			const auto viewport = n_render_system.m_viewport;

			auto hybrid_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<RTHybridData>());
			auto gbuffer_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<DeferredMainTaskData>());

			// Filtered output
			{
				auto cpu_handle = data.allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::spatial_reconstruction, params::SpatialReconstructionE::OUTPUT)));
				d3d12::CreateUAVFromSpecificRTV(n_render_target, cpu_handle, 0, n_render_target->m_create_info.m_rtv_formats[0]);
			}
			// Reflection buffer input
			{
				auto cpu_handle = data.allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::spatial_reconstruction, params::SpatialReconstructionE::REFLECTION_BUFFER)));
				d3d12::CreateSRVFromSpecificRTV(hybrid_rt, cpu_handle, 0, hybrid_rt->m_create_info.m_rtv_formats[0]);
				d3d12::CreateSRVFromSpecificRTV(hybrid_rt, cpu_handle, 2, hybrid_rt->m_create_info.m_rtv_formats[2]);
			}
			// G-Buffer input
			{
				auto cpu_handle = data.allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::spatial_reconstruction, params::SpatialReconstructionE::GBUFFERS)));
				d3d12::CreateSRVFromSpecificRTV(gbuffer_rt, cpu_handle, 0, gbuffer_rt->m_create_info.m_rtv_formats[0]);
				d3d12::CreateSRVFromSpecificRTV(gbuffer_rt, cpu_handle, 1, gbuffer_rt->m_create_info.m_rtv_formats[1]);
				d3d12::CreateSRVFromDSV(gbuffer_rt, cpu_handle);
			}

			d3d12::BindComputePipeline(cmd_list, data.pipeline);

			// Update camera constant buffer
			auto camera = sg.GetActiveCamera();
			temp::SpatialReconstructionCameraData cam_data;
			cam_data.inv_view = DirectX::XMMatrixInverse(nullptr, camera->m_view);
			cam_data.inv_projection = DirectX::XMMatrixInverse(nullptr, camera->m_projection);
			cam_data.view = camera->m_view;
			n_render_system.m_camera_pool->Update(data.camera_cb, sizeof(temp::SpatialReconstructionCameraData), 0, frame_idx, (std::uint8_t*)&cam_data);

			d3d12::BindComputeConstantBuffer(cmd_list, data.camera_cb->m_native, 0, frame_idx);

			{
				constexpr unsigned int dest_idx = rs_layout::GetHeapLoc(params::spatial_reconstruction, params::SpatialReconstructionE::OUTPUT);
				auto handle_uav = data.allocation.GetDescriptorHandle(dest_idx);
				d3d12::SetShaderUAV(cmd_list, 1, dest_idx, handle_uav);
			}

			{
				constexpr unsigned int dest_idx = rs_layout::GetHeapLoc(params::spatial_reconstruction, params::SpatialReconstructionE::REFLECTION_BUFFER);
				auto handle_srv = data.allocation.GetDescriptorHandle(dest_idx);
				d3d12::SetShaderSRV(cmd_list, 1, dest_idx, handle_srv, 2);
			}

			{
				constexpr unsigned int dest_idx = rs_layout::GetHeapLoc(params::spatial_reconstruction, params::SpatialReconstructionE::GBUFFERS);
				auto handle_srv = data.allocation.GetDescriptorHandle(dest_idx);
				d3d12::SetShaderSRV(cmd_list, 1, dest_idx, handle_srv, 3);
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

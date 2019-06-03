#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "d3d12_deferred_main.hpp"
#include "../d3d12/d3d12_structured_buffer_pool.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../scene_graph/scene_graph.hpp"
#include "../scene_graph/camera_node.hpp"
#include "../engine_registry.hpp"

#include "../render_tasks/d3d12_deferred_main.hpp"
#include "../render_tasks/d3d12_build_acceleration_structures.hpp"
#include "../imgui_tools.hpp"

namespace wr
{
	struct RTReflectionData
	{
		RTHybridData base_data;
	};

	namespace internal
	{
		inline void SetupRTReflectionTask(RenderSystem & render_system, FrameGraph & fg, RenderTaskHandle & handle, bool resize)
		{
			// Initialize variables
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);
			auto& device = n_render_system.m_device;
			auto& data = fg.GetData<RTReflectionData>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);
			d3d12::SetName(n_render_target, L"Raytracing Reflection Target");

			if (!resize)
			{
				auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);

				// Get AS build data
				auto& as_build_data = fg.GetPredecessorData<wr::ASBuildData>();

				data.base_data.out_output_alloc = std::move(as_build_data.out_allocator->Allocate(3));
				data.base_data.out_gbuffer_albedo_alloc = std::move(as_build_data.out_allocator->Allocate());
				data.base_data.out_gbuffer_normal_alloc = std::move(as_build_data.out_allocator->Allocate());
				data.base_data.out_gbuffer_depth_alloc = std::move(as_build_data.out_allocator->Allocate());
				
				data.base_data.tlas_requires_init = true;
			}

			CreateUAVsAndSRVs(fg, data.base_data, n_render_target);

			if (!resize)
			{
				// Camera constant buffer
				data.base_data.out_cb_camera_handle = static_cast<D3D12ConstantBufferHandle*>(n_render_system.m_raytracing_cb_pool->Create(sizeof(temp::RTHybridCamera_CBData)));

				// Pipeline State Object
				auto& rt_registry = RTPipelineRegistry::Get();
				data.base_data.out_state_object = static_cast<d3d12::StateObject*>(rt_registry.Find(state_objects::rt_reflection_state_object));

				// Root Signature
				auto& rs_registry = RootSignatureRegistry::Get();
				data.base_data.out_root_signature = static_cast<d3d12::RootSignature*>(rs_registry.Find(root_signatures::rt_hybrid_global));
			}

			// Create Shader Tables
			for (int i = 0; i < d3d12::settings::num_back_buffers; ++i)
			{
				CreateShaderTables(device, data.base_data, "ReflectionRaygenEntry", i);
			}

			// Setup frame index
			data.base_data.frame_idx = 0;
		}

		inline void ExecuteRTReflectionTask(RenderSystem & render_system, FrameGraph & fg, SceneGraph & scene_graph, RenderTaskHandle & handle)
		{
			// Initialize variables
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
			auto& data = fg.GetData<RTReflectionData>(handle);
			
			Render(n_render_system, fg, scene_graph, data.base_data, cmd_list, handle, "ReflectionRaygenEntry");			
		}

		inline void DestroyRTReflectionTask(FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			auto& data = fg.GetData<RTReflectionData>(handle);

			if (!resize)
			{
				// Small hack to force the allocations to go out of scope, which will tell the allocator to free them
				std::move(data.base_data.out_output_alloc);
				std::move(data.base_data.out_gbuffer_albedo_alloc);
				std::move(data.base_data.out_gbuffer_normal_alloc);
				std::move(data.base_data.out_gbuffer_depth_alloc);
			}
		}

	} /* internal */

	inline void AddRTReflectionTask(FrameGraph& fg)
	{
		std::wstring name(L"Hybrid raytracing reflections");

		RenderTargetProperties rt_properties
		{
			RenderTargetProperties::IsRenderWindow(false),
			RenderTargetProperties::Width(std::nullopt),
			RenderTargetProperties::Height(std::nullopt),
			RenderTargetProperties::ExecuteResourceState(ResourceState::UNORDERED_ACCESS),
			RenderTargetProperties::FinishedResourceState(ResourceState::COPY_SOURCE),
			RenderTargetProperties::CreateDSVBuffer(false),
			RenderTargetProperties::DSVFormat(Format::UNKNOWN),
			RenderTargetProperties::RTVFormats({ wr::Format::R16G16B16A16_FLOAT, wr::Format::R8_UNORM, wr::Format::R16G16B16A16_FLOAT }),
			RenderTargetProperties::NumRTVFormats(3),
			RenderTargetProperties::Clear(true),
			RenderTargetProperties::ClearDepth(true),
			RenderTargetProperties::ResolutionScalar(0.5f)
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			internal::SetupRTReflectionTask(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle)
		{
			internal::ExecuteRTReflectionTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			internal::DestroyRTReflectionTask(fg, handle, resize);
		};

		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		fg.AddTask<RTReflectionData>(desc, name, FG_DEPS(1, DeferredMainTaskData));
	}

} /* wr */
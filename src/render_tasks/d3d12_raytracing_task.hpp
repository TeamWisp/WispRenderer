#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "../d3d12/d3d12_structured_buffer_pool.hpp"
#include "../frame_graph/render_task.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../scene_graph/camera_node.hpp"
#include "../d3d12/d3d12_rt_pipeline_registry.hpp"
#include "../engine_registry.hpp"

#include "../render_tasks/d3d12_deferred_main.hpp"

namespace wr
{
	struct RaytracingData
	{
		d3d12::DescriptorHeap* out_rt_heap;
		d3d12::StagingBuffer* out_vb;

		d3d12::ShaderTable* out_raygen_shader_table;
		d3d12::ShaderTable* out_miss_shader_table;
		d3d12::ShaderTable* out_hitgroup_shader_table;
		
		bool out_init;
	};
	using RaytracingTask = RenderTask<RaytracingData>;

	namespace internal
	{

		inline void SetupRaytracingTask(RenderSystem & render_system, RaytracingTask & task, RaytracingData & data)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);
			auto& device = n_render_system.m_device;

			data.out_init = true;

			// top level, bottom level and output buffer. (even though I don't use bottom level.)
			d3d12::desc::DescriptorHeapDesc heap_desc;
			heap_desc.m_num_descriptors = 3;
			heap_desc.m_type = DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV;
			heap_desc.m_shader_visible = true;
			heap_desc.m_versions = 1;
			data.out_rt_heap = d3d12::CreateDescriptorHeap(device, heap_desc);

			// Create vertex buffer
			float aspect_ratio = 1280.f / 720.f;
			std::vector<DirectX::XMVECTOR> tri_vertices =
			{
				{ 0.0f, 0.25f * aspect_ratio, 0.f },
				{ 0.25f, -0.25f * aspect_ratio, 0.f },
				{ -0.25f, -0.25f * aspect_ratio, 0.f },
			};

			float stride = sizeof(decltype(tri_vertices[0]));
			data.out_vb = d3d12::CreateStagingBuffer(device, nullptr, stride * tri_vertices.size(), stride, ResourceState::NON_PIXEL_SHADER_RESOURCE);
		
			data.out_vb->m_buffer->SetName(L"RT VB Buffer");
			data.out_vb->m_staging->SetName(L"RT VB Staging Buffer");

			auto rt_registry = RTPipelineRegistry::Get();
			auto state_obj = static_cast<D3D12StateObject*>(rt_registry.Find(state_objects::state_object));

			// Raygen Shader Table
			{
				// Create Record(s)
				UINT shader_record_count = 1;
				auto shader_identifier_size = d3d12::GetShaderIdentifierSize(device, state_obj->m_native);
				auto shader_identifier = d3d12::GetShaderIdentifier(device, state_obj->m_native, "RaygenEntry");

				auto shader_record = d3d12::CreateShaderRecord(shader_identifier, shader_identifier_size);

				// Create Table
				data.out_raygen_shader_table = d3d12::CreateShaderTable(device, shader_record_count, shader_identifier_size);
				d3d12::AddShaderRecord(data.out_raygen_shader_table, shader_record);
			}

			// Miss Shader Table
			{
				// Create Record(s)
				UINT shader_record_count = 1;
				auto shader_identifier_size = d3d12::GetShaderIdentifierSize(device, state_obj->m_native);
				auto shader_identifier = d3d12::GetShaderIdentifier(device, state_obj->m_native, "MissEntry");

				auto shader_record = d3d12::CreateShaderRecord(shader_identifier, shader_identifier_size);

				// Create Table
				data.out_miss_shader_table = d3d12::CreateShaderTable(device, shader_record_count, shader_identifier_size);
				d3d12::AddShaderRecord(data.out_miss_shader_table, shader_record);
			}

			// Hit Group Shader Table
			{
				// Create Record(s)
				UINT shader_record_count = 1;
				auto shader_identifier_size = d3d12::GetShaderIdentifierSize(device, state_obj->m_native);
				auto shader_identifier = d3d12::GetShaderIdentifier(device, state_obj->m_native, "MyHitGroup");

				auto shader_record = d3d12::CreateShaderRecord(shader_identifier, shader_identifier_size);

				// Create Table
				data.out_hitgroup_shader_table = d3d12::CreateShaderTable(device, shader_record_count, shader_identifier_size);
				d3d12::AddShaderRecord(data.out_hitgroup_shader_table, shader_record);
			}
		}

		inline void ExecuteRaytracingTask(RenderSystem & render_system, RaytracingTask & task, SceneGraph & scene_graph, RaytracingData & data)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);
			auto device = n_render_system.m_device;
			auto cmd_list = task.GetCommandList<d3d12::CommandList>().first;

			if (n_render_system.m_render_window.has_value())
			{
				// Initialize requirements
				if (data.out_init)
				{
					d3d12::StageBuffer(data.out_vb, cmd_list);

					d3d12::CreateAccelerationStructures(device, cmd_list, data.out_rt_heap, std::vector<d3d12::StagingBuffer*>{ data.out_vb });

					data.out_init = false;
				}

				d3d12::BindDescriptorHeaps(cmd_list, { data.out_rt_heap }, 0);

				// Setup the raytracing task
				D3D12_DISPATCH_RAYS_DESC desc = {};
				desc.HitGroupTable.StartAddress = data.out_hitgroup_shader_table->m_resource->GetGPUVirtualAddress();
				desc.HitGroupTable.SizeInBytes = data.out_hitgroup_shader_table->m_resource->GetDesc().Width;
				desc.HitGroupTable.StrideInBytes = desc.HitGroupTable.SizeInBytes;

				desc.MissShaderTable.StartAddress = data.out_miss_shader_table->m_resource->GetGPUVirtualAddress();
				desc.MissShaderTable.SizeInBytes = data.out_miss_shader_table->m_resource->GetDesc().Width;
				desc.MissShaderTable.StrideInBytes = desc.MissShaderTable.SizeInBytes;

				desc.RayGenerationShaderRecord.StartAddress = data.out_raygen_shader_table->m_resource->GetGPUVirtualAddress();
				desc.RayGenerationShaderRecord.SizeInBytes = data.out_hitgroup_shader_table->m_resource->GetDesc().Width;
				// Dimensions of the image to render, identical to a window dimensions
				desc.Width = 1280;
				desc.Height = 720;


				cmd_list->m_native->DispatchRays(&desc);
			}
		}

		inline void DestroyRaytracingTask(RaytracingTask & task, RaytracingData& data)
		{
		}

	} /* internal */


	//! Used to create a new defferred task.
	[[nodiscard]] inline std::unique_ptr<RaytracingTask> GetRaytracingTask()
	{
		auto ptr = std::make_unique<RaytracingTask>(nullptr, "Deferred Render Task", RenderTaskType::DIRECT, true,
			RenderTargetProperties{
				true,
				std::nullopt,
				std::nullopt,
				std::nullopt,
				std::nullopt,
				false,
				Format::UNKNOWN,
				{ Format::R8G8B8A8_UNORM },
				1,
				true,
				true
			},
			[](RenderSystem & render_system, RaytracingTask & task, RaytracingData & data, bool) { internal::SetupRaytracingTask(render_system, task, data); },
			[](RenderSystem & render_system, RaytracingTask & task, SceneGraph & scene_graph, RaytracingData & data) { internal::ExecuteRaytracingTask(render_system, task, scene_graph, data); },
			[](RaytracingTask & task, RaytracingData & data, bool) { internal::DestroyRaytracingTask(task, data); }
		);

		return ptr;
	}

} /* wr */

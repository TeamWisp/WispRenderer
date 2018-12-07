#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "../d3d12/d3d12_structured_buffer_pool.hpp"
#include "../frame_graph/render_task.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../scene_graph/camera_node.hpp"
#include "../d3d12/d3d12_pipeline_registry.hpp"
#include "../engine_registry.hpp"

#include "../render_tasks/d3d12_deferred_main.hpp"

namespace wr
{
	struct RaytracingData
	{
		d3d12::DescriptorHeap* out_rt_heap;
		d3d12::StagingBuffer* out_vb;
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
			data.out_vb = d3d12::CreateStagingBuffer(device, nullptr, stride * tri_vertices.size(), stride, /*ResourceState::VERTEX_AND_CONSTANT_BUFFER*/ ResourceState::NON_PIXEL_SHADER_RESOURCE);
		}

		inline void ExecuteRaytracingTask(RenderSystem & render_system, RaytracingTask & task, SceneGraph & scene_graph, RaytracingData & data)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(render_system);
			auto device = n_render_system.m_device;
			auto cmd_list = task.GetCommandList<D3D12CommandList>().first;

			if (n_render_system.m_render_window.has_value())
			{
				// Initialize requirements
				if (data.out_init)
				{
					d3d12::StageBuffer(data.out_vb, cmd_list);

					d3d12::CreateAccelerationStructures(device, cmd_list, data.out_rt_heap, std::vector<d3d12::StagingBuffer*>{ data.out_vb });

					data.out_init = false;
				}
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

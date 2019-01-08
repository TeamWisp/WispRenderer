#pragma once

#include "../d3d12/d3d12_structs.hpp"
#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../frame_graph/frame_graph.hpp"

namespace wr
{
	struct RenderTargetReadBackTaskData
	{
		// Render target of the previous render task (should be the output from the composition task
		d3d12::RenderTarget* predecessor_render_target;

		// Read back buffer used to retrieve the pixel data on the GPU
		d3d12::ReadbackBuffer* readback_buffer;
	};

	namespace internal
	{
		// Utility function to keep the execution function clean
		inline void MapReadbackBuffer(d3d12::ReadbackBuffer* const readback_buffer, std::uint64_t buffer_size)
		{
			D3D12_RANGE readback_buffer_range{ 0, buffer_size };

			HRESULT res = readback_buffer->m_resource->Map(
				0,
				&readback_buffer_range,
				reinterpret_cast<void**>(&readback_buffer->m_data));

			if (FAILED(res))
				LOGC("Failed to map readback buffer to range...");
		}

		// Utility function to keep the destruction function clean
		inline void UnmapReadbackBuffer(d3d12::ReadbackBuffer* const readback_buffer)
		{
			D3D12_RANGE emptyRange{ 0, 0 };
			readback_buffer->m_resource->Unmap(
				0,
				&emptyRange);
		}
		
		template<typename T>
		inline void SetupReadBackTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, float* out_buffer_data, std::uint64_t& out_buffer_size)
		{
			auto& data = fg.GetData<RenderTargetReadBackTaskData>(handle);
			auto& dx12_render_system = static_cast<D3D12RenderSystem&>(rs);

			// Save the previous render target for use in the execute function
			data.predecessor_render_target = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<T>());

			// Information about the previous render target
			wr::d3d12::desc::RenderTargetDesc predecessor_rt_desc = data.predecessor_render_target->m_create_info;

			// Bytes per pixel of the previous render target
			unsigned int bytesPerPixel = BytesPerPixel(predecessor_rt_desc.m_rtv_formats[0]);

			// Information for creating the read back buffer object
			wr::d3d12::desc::ReadbackDesc readback_description = {};
			readback_description.m_initial_state = ResourceState::COPY_DEST;
			readback_description.m_buffer_size = dx12_render_system.m_viewport.m_viewport.Width * dx12_render_system.m_viewport.m_viewport.Height * bytesPerPixel;
			
			// Create the actual read back buffer
			data.readback_buffer = d3d12::CreateReadbackBuffer(dx12_render_system.m_device, &readback_description);

			// Map the read back buffer to a CPU pointer to allow the application to read the frame pixel data out of RAM
			MapReadbackBuffer(data.readback_buffer, readback_description.m_buffer_size);

			// Allow the application to access the data outsize of the render task by pointing to the original data
			// The user does not have to worry about deallocating the memory. Just check whether it is a nullptr...
			out_buffer_data = data.readback_buffer->m_data;
			out_buffer_size = readback_description.m_buffer_size;
		}

		inline void ExecuteReadBackTask(RenderSystem& render_system, FrameGraph& frame_graph, SceneGraph& scene_graph, RenderTaskHandle handle)
		{
			auto& dx12_render_system = static_cast<D3D12RenderSystem&>(render_system);
			auto& data = frame_graph.GetData<RenderTargetReadBackTaskData>(handle);
			auto command_list = frame_graph.GetCommandList<d3d12::CommandList>(handle);

			const auto frame_index = dx12_render_system.GetFrameIdx();

			// Copy the GPU data to the read back buffer, making it available to the CPU
			command_list->m_native->CopyResource(data.readback_buffer->m_resource, data.predecessor_render_target->m_render_targets[0]);
		}
		
		inline void DestroyReadBackTask(FrameGraph& fg, RenderTaskHandle handle)
		{
			// Data used for this render task
			auto& data = fg.GetData<RenderTargetReadBackTaskData>(handle);

			// Clean up the mapping
			UnmapReadbackBuffer(data.readback_buffer);

			// Clean up the read back buffer
			Destroy(data.readback_buffer);
		}

	} /* internal */

	template<typename T>
	inline void AddRenderTargetReadBackTask(FrameGraph& frame_graph, std::optional<unsigned int> target_width, std::optional<unsigned int> target_height, float* out_buffer_data, std::uint64_t& out_buffer_size)
	{
		// This is the same as the composition task, as this task should not change anything of the buffer that comes
		// into the task. It just copies the data to the read back buffer and leaves the render target be.
		RenderTargetProperties rt_properties{
			false,						// This pass does not use the render window
			target_width,				// Width of the frame
			target_height,				// Height of the frame
			ResourceState::COPY_SOURCE,	// Execution state
			ResourceState::COPY_SOURCE,	// Finished state
			false,						// Do not create a depth-stencil-view
			Format::UNKNOWN,			// Depth-stencil-view format
			{ Format::R8G8B8A8_UNORM },	// Render target array containing all formats for this render target
			1,							// Number of render target formats
			true,						// Clear flag
			true						// Clear depth flag
		};

		// Render task information
		RenderTaskDesc render_task_description;

		// Set-up
		render_task_description.m_setup_func = [&](RenderSystem& render_system, FrameGraph& frame_graph, RenderTaskHandle render_task_handle, bool) {
			internal::SetupReadBackTask<T>(render_system, frame_graph, render_task_handle, out_buffer_data, out_buffer_size);
		};

		// Execution
		render_task_description.m_execute_func = [](RenderSystem& render_system, FrameGraph& frame_graph, SceneGraph& scene_graph, RenderTaskHandle handle) {
			internal::ExecuteReadBackTask(render_system, frame_graph, scene_graph, handle);
		};

		// Destruction and clean-up
		render_task_description.m_destroy_func = [](FrameGraph& frame_graph, RenderTaskHandle handle, bool) {
			internal::DestroyReadBackTask(frame_graph, handle);
		};

		render_task_description.m_name = std::string(std::string("Render Target (") + std::string(typeid(T).name()) + std::string(") read-back task")).c_str();
		render_task_description.m_properties = rt_properties;
		render_task_description.m_type = RenderTaskType::COPY;
		render_task_description.m_allow_multithreading = true;

		// Save this task to the frame graph system
		frame_graph.AddTask<RenderTargetReadBackTaskData>(render_task_description);
	}

} /* wr */

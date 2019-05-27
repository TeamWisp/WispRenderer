#pragma once

#include "../d3d12/d3d12_enums.hpp"
#include "../d3d12/d3d12_defines.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_structs.hpp"
#include "../frame_graph/frame_graph.hpp"

namespace wr
{
	struct PixelReadbackTaskData
	{
		// Render target of the previous render task (should be the output from the composition task
		d3d12::RenderTarget* predecessor_render_target = nullptr;

		// Read back buffer used to retrieve the depth data on the GPU
		d3d12::ReadbackBufferResource* readback_buffer = nullptr;

		// Stores the final depth data
		CPUTexture cpu_texture_output;
	};

	namespace internal
	{
		template<typename T>
		inline void SetupPixelDataReadBackTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle)
		{
			auto& data = fg.GetData<PixelReadbackTaskData>(handle);
			auto& dx12_render_system = static_cast<D3D12RenderSystem&>(rs);

			// Save the previous render target for use in the execute function
			data.predecessor_render_target = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<T>());

			// Information about the previous render target
			wr::d3d12::desc::RenderTargetDesc predecessor_rt_desc = data.predecessor_render_target->m_create_info;

			// Bytes per pixel of the previous render target
			unsigned int bytesPerPixel = BytesPerPixel(predecessor_rt_desc.m_rtv_formats[0]);

			// Information for creating the read back buffer object
			unsigned int rt_width = d3d12::GetRenderTargetWidth(data.predecessor_render_target);
			unsigned int rt_height = d3d12::GetRenderTargetHeight(data.predecessor_render_target);
			std::uint32_t aligned_buffer_size = SizeAlignTwoPower(rt_width * bytesPerPixel, 256) * rt_height;

			// Create the actual read back buffer
			data.readback_buffer = d3d12::CreateReadbackBuffer(dx12_render_system.m_device, aligned_buffer_size);
			d3d12::SetName(data.readback_buffer, L"Pixel data read back render pass");

			// Keep the read back buffer mapped for the duration of the entire application
			data.cpu_texture_output.m_data = reinterpret_cast<float*>(MapReadbackBuffer(data.readback_buffer, aligned_buffer_size));
			data.cpu_texture_output.m_buffer_width = rt_width;
			data.cpu_texture_output.m_buffer_height = rt_height;
			data.cpu_texture_output.m_bytes_per_pixel = bytesPerPixel;
 		}

		inline void ExecutePixelDataReadBackTask(RenderSystem& render_system, FrameGraph& frame_graph, SceneGraph& scene_graph, RenderTaskHandle handle)
		{
			auto& dx12_render_system = static_cast<D3D12RenderSystem&>(render_system);
			auto& data = frame_graph.GetData<PixelReadbackTaskData>(handle);
			auto command_list = frame_graph.GetCommandList<d3d12::CommandList>(handle);

			D3D12_TEXTURE_COPY_LOCATION destination = {};
			destination.pResource = data.readback_buffer->m_resource;
			destination.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			destination.SubresourceIndex = 0;
			destination.PlacedFootprint.Footprint.Format = static_cast<DXGI_FORMAT>(data.predecessor_render_target->m_create_info.m_rtv_formats[0]);
			destination.PlacedFootprint.Footprint.Width = static_cast<std::uint32_t>(dx12_render_system.m_viewport.m_viewport.Width);
			destination.PlacedFootprint.Footprint.Height = static_cast<std::uint32_t>(dx12_render_system.m_viewport.m_viewport.Height);
			destination.PlacedFootprint.Footprint.Depth = 1;

			std::uint32_t row_pitch = destination.PlacedFootprint.Footprint.Width * BytesPerPixel(data.predecessor_render_target->m_create_info.m_rtv_formats[0]);
			std::uint32_t aligned_row_pitch = SizeAlignTwoPower(row_pitch, 256);	// 256 byte aligned

			destination.PlacedFootprint.Footprint.RowPitch = aligned_row_pitch;

			D3D12_TEXTURE_COPY_LOCATION source = {};
			source.pResource = data.predecessor_render_target->m_render_targets[0];
			source.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			source.SubresourceIndex = 0;

			// Copy pixel data
			command_list->m_native->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);

			// Update the frame graph output texture (allows the renderer to access this data)
			frame_graph.SetOutputTexture(data.cpu_texture_output, CPUTextureType::PIXEL_DATA);
		}
		
		inline void DestroyPixelDataReadBackTask(FrameGraph& fg, RenderTaskHandle handle)
		{
			// Data used for this render task
			auto& data = fg.GetData<PixelReadbackTaskData>(handle);

			// Clean up the read back buffer
			UnmapReadbackBuffer(data.readback_buffer);
			Destroy(data.readback_buffer);
		}

	} /* internal */

	template<typename T>
	inline void AddPixelDataReadBackTask(FrameGraph& frame_graph, std::optional<unsigned int> target_width, std::optional<unsigned int> target_height)
	{
		std::wstring name(L"Pixel Data CPU Readback");

		// This is the same as the composition task, as this task should not change anything of the buffer that comes
		// into the task. It just copies the data to the read back buffer and leaves the render target be.
		RenderTargetProperties rt_properties{
			RenderTargetProperties::IsRenderWindow(false),
			RenderTargetProperties::Width(target_width),
			RenderTargetProperties::Height(target_height),
			RenderTargetProperties::ExecuteResourceState(ResourceState::COPY_SOURCE),
			RenderTargetProperties::FinishedResourceState(ResourceState::RENDER_TARGET),
			RenderTargetProperties::CreateDSVBuffer(false),
			RenderTargetProperties::DSVFormat(Format::UNKNOWN),
			RenderTargetProperties::RTVFormats({ Format::R8G8B8A8_UNORM }),
			RenderTargetProperties::NumRTVFormats(1),
			RenderTargetProperties::Clear(false),
			RenderTargetProperties::ClearDepth(false),
			RenderTargetProperties::ResourceName(name)
		};

		// Render task information
		RenderTaskDesc readback_task_description;

		// Set-up
		readback_task_description.m_setup_func = [](RenderSystem& render_system, FrameGraph& frame_graph, RenderTaskHandle render_task_handle, bool) {
			internal::SetupPixelDataReadBackTask<T>(render_system, frame_graph, render_task_handle);
		};

		// Execution
		readback_task_description.m_execute_func = [](RenderSystem& render_system, FrameGraph& frame_graph, SceneGraph& scene_graph, RenderTaskHandle handle) {
			internal::ExecutePixelDataReadBackTask(render_system, frame_graph, scene_graph, handle);
		};

		// Destruction and clean-up
		readback_task_description.m_destroy_func = [](FrameGraph& frame_graph, RenderTaskHandle handle, bool) {
			internal::DestroyPixelDataReadBackTask(frame_graph, handle);
		};

		readback_task_description.m_properties = rt_properties;
		readback_task_description.m_type = RenderTaskType::COPY;
		readback_task_description.m_allow_multithreading = false;

		// Save this task to the frame graph system
		frame_graph.AddTask<PixelReadbackTaskData>(readback_task_description);
	}

} /* wr */

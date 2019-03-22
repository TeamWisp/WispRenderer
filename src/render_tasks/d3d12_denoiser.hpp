#pragma once

#include "../frame_graph/frame_graph.hpp"
#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "../d3d12/d3d12_structs.hpp"
#include <optix.h>

namespace wr
{
	struct DenoiserTaskData
	{
		RTcontext m_optix_context;

		RTbuffer m_denoiser_input;
		RTbuffer m_denoiser_output;

		RTpostprocessingstage m_denoiser_post_processor;
		RTvariable m_denoiser_post_processor_input;
		RTvariable m_denoiser_post_processor_output;
		RTvariable m_denoiser_post_processor_hdr;

		RTcommandlist m_denoiser_command_list;

		d3d12::RenderTarget* m_prev_output;

		d3d12::ReadbackBufferResource* m_readback_buffer;

		d3d12::Fence* m_readback_finished_fence;

		ID3D12Resource* m_upload_buffer;

		d3d12::CommandList* m_download_cmd_list;
	};

	namespace internal
	{
		template<typename T>
		inline void SetupDenoiserTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<DenoiserTaskData>(handle);

			data.m_prev_output = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<T>());

			if (!resize)
			{
				RTresult res = rtContextCreate(&data.m_optix_context);
				if (res != RT_SUCCESS)
				{
					LOGE("Error initializing optix context");
				}
			}

			d3d12::desc::ReadbackDesc read_back_desc = {};
			read_back_desc.m_buffer_width = n_render_system.m_viewport.m_viewport.Width;
			read_back_desc.m_buffer_height = n_render_system.m_viewport.m_viewport.Height;
			read_back_desc.m_bytes_per_pixel = BytesPerPixel(data.m_prev_output->m_create_info.m_rtv_formats[0]);

			data.m_readback_buffer = d3d12::CreateReadbackBuffer(n_render_system.m_device, &read_back_desc);

			rtBufferCreate(data.m_optix_context, RT_BUFFER_INPUT, &data.m_denoiser_input);
			rtBufferSetFormat(data.m_denoiser_input, RT_FORMAT_FLOAT4);
			rtBufferSetSize2D(data.m_denoiser_input, read_back_desc.m_buffer_width, read_back_desc.m_buffer_height);

			rtBufferCreate(data.m_optix_context, RT_BUFFER_OUTPUT, &data.m_denoiser_output);
			rtBufferSetFormat(data.m_denoiser_output, RT_FORMAT_FLOAT4);
			rtBufferSetSize2D(data.m_denoiser_output, read_back_desc.m_buffer_width, read_back_desc.m_buffer_height);

			D3D12_RESOURCE_DESC desc = fg.GetRenderTarget<d3d12::RenderTarget>(handle)->m_render_targets[0]->GetDesc();

			uint64_t textureUploadBufferSize;
			n_render_system.m_device->m_native->GetCopyableFootprints(
				&desc,
				0,
				desc.MipLevels * desc.DepthOrArraySize,
				0,
				nullptr,
				nullptr,
				nullptr,
				&textureUploadBufferSize);

			CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);

			n_render_system.m_device->m_native->CreateCommittedResource(
				&uploadHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&data.m_upload_buffer));

			if (true)
			{

				rtPostProcessingStageCreateBuiltin(data.m_optix_context, "DLDenoiser", &data.m_denoiser_post_processor);

				rtPostProcessingStageDeclareVariable(data.m_denoiser_post_processor, "input_buffer", &data.m_denoiser_post_processor_input);
				rtPostProcessingStageDeclareVariable(data.m_denoiser_post_processor, "output_buffer", &data.m_denoiser_post_processor_output);

				RTresult res = rtPostProcessingStageDeclareVariable(data.m_denoiser_post_processor, "hdr", &data.m_denoiser_post_processor_hdr);
				if (res != RT_SUCCESS)
				{
					LOGW("Couldn't find optix variable");
				}

			}

			rtVariableSetObject(data.m_denoiser_post_processor_input, data.m_denoiser_input);
			rtVariableSetObject(data.m_denoiser_post_processor_output, data.m_denoiser_output);
			rtVariableSet1ui(data.m_denoiser_post_processor_hdr, 1);

			rtCommandListCreate(data.m_optix_context, &data.m_denoiser_command_list);

			rtCommandListAppendPostprocessingStage(data.m_denoiser_command_list,
				data.m_denoiser_post_processor,
				read_back_desc.m_buffer_width,
				read_back_desc.m_buffer_height);

			rtCommandListFinalize(data.m_denoiser_command_list);

			if (!resize)
			{
				data.m_download_cmd_list = d3d12::CreateCommandList(n_render_system.m_device, 1, CmdListType::CMD_LIST_DIRECT);
			}

			d3d12::Begin(data.m_download_cmd_list, 0);

			D3D12_TEXTURE_COPY_LOCATION destination = {};
			destination.pResource = data.m_readback_buffer->m_resource;
			destination.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			destination.SubresourceIndex = 0;
			destination.PlacedFootprint.Footprint.Format = static_cast<DXGI_FORMAT>(data.m_prev_output->m_create_info.m_rtv_formats[0]);
			destination.PlacedFootprint.Footprint.Width = read_back_desc.m_buffer_width;
			destination.PlacedFootprint.Footprint.Height = read_back_desc.m_buffer_height;
			destination.PlacedFootprint.Footprint.Depth = 1;

			std::uint32_t row_pitch = destination.PlacedFootprint.Footprint.Width * BytesPerPixel(data.m_prev_output->m_create_info.m_rtv_formats[0]);
			std::uint32_t aligned_row_pitch = SizeAlignTwoPower(row_pitch, 256);	// 256 byte aligned

			destination.PlacedFootprint.Footprint.RowPitch = aligned_row_pitch;

			D3D12_TEXTURE_COPY_LOCATION source = {};
			source.pResource = data.m_prev_output->m_render_targets[0];
			source.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			source.SubresourceIndex = 0;

			// Copy pixel data
			data.m_download_cmd_list->m_native->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);

			d3d12::End(data.m_download_cmd_list);

			if (!resize)
			{
				data.m_readback_finished_fence = d3d12::CreateFence(n_render_system.m_device);
			}
		}

		inline void ExecuteDenoiserTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& scene_graph, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<DenoiserTaskData>(handle);
			auto render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);
			auto command_list = fg.GetCommandList<d3d12::CommandList>(handle);

			d3d12::Execute(n_render_system.m_direct_queue, { data.m_download_cmd_list }, data.m_readback_finished_fence);

			d3d12::WaitFor(data.m_readback_finished_fence);

			if (BytesPerPixel(data.m_prev_output->m_create_info.m_rtv_formats[0]) == 4) {
				unsigned char* readback_source = reinterpret_cast<unsigned char*>(d3d12::MapReadbackBuffer(data.m_readback_buffer, data.m_readback_buffer->m_size));

					float* rt_dest;
					void* temp;
					rtBufferMap(data.m_denoiser_input, &temp);

					rt_dest = reinterpret_cast<float*>(temp);

					for (int i = 0; i < data.m_readback_buffer->m_desc.m_buffer_width*data.m_readback_buffer->m_desc.m_buffer_height * 4; ++i)
					{
						rt_dest[i] = static_cast<float>(readback_source[i]) / 255.f;
					}

				rtBufferUnmap(data.m_denoiser_input);
				d3d12::UnmapReadbackBuffer(data.m_readback_buffer);

			}
			else
			{
				float* readback_source = reinterpret_cast<float*>(d3d12::MapReadbackBuffer(data.m_readback_buffer, data.m_readback_buffer->m_size));

				float* rt_dest;
				void* temp;
				rtBufferMap(data.m_denoiser_input, &temp);

				rt_dest = reinterpret_cast<float*>(temp);

				float max_value = 0.f;
				float min_value = 0.f;
				for (int i = 0; i < data.m_readback_buffer->m_desc.m_buffer_width*data.m_readback_buffer->m_desc.m_buffer_height * 4; ++i)
				{
					rt_dest[i] = readback_source[i];
					min_value = (min_value > readback_source[i] || min_value == 0.f) ? readback_source[i] : min_value;
					max_value = (max_value < readback_source[i] || max_value == 0.f) ? readback_source[i] : max_value;
				}

				rtBufferUnmap(data.m_denoiser_input);
				d3d12::UnmapReadbackBuffer(data.m_readback_buffer);
			}

			rtCommandListExecute(data.m_denoiser_command_list);

			float* rt_source;
			unsigned char* upload_dest;
			void* temp;

			rtBufferMap(data.m_denoiser_output, &temp);

			rt_source = reinterpret_cast<float*>(temp);

			data.m_upload_buffer->Map(0, nullptr, &temp);

			upload_dest = reinterpret_cast<unsigned char*>(temp);

			for (int i = 0; i < data.m_readback_buffer->m_desc.m_buffer_width*data.m_readback_buffer->m_desc.m_buffer_height * 4; ++i)
			{
				upload_dest[i] = static_cast<unsigned char>(rt_source[i] * 255.f);
			}

			rtBufferUnmap(data.m_denoiser_output);
			data.m_upload_buffer->Unmap(0, nullptr);

			D3D12_TEXTURE_COPY_LOCATION copy_source = {};
			copy_source.pResource = data.m_upload_buffer;
			copy_source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			copy_source.PlacedFootprint.Offset = 0;
			copy_source.PlacedFootprint.Footprint.Depth = 1;
			copy_source.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			copy_source.PlacedFootprint.Footprint.Height = data.m_readback_buffer->m_desc.m_buffer_height;
			copy_source.PlacedFootprint.Footprint.Width = data.m_readback_buffer->m_desc.m_buffer_width;
			copy_source.PlacedFootprint.Footprint.RowPitch = SizeAlignTwoPower(data.m_readback_buffer->m_desc.m_buffer_width * 4, 256);

			D3D12_TEXTURE_COPY_LOCATION copy_dest = {};
			copy_dest.pResource = render_target->m_render_targets[0];
			copy_dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			copy_dest.SubresourceIndex = 0;

			command_list->m_native->CopyTextureRegion(&copy_dest, 0, 0, 0, &copy_source, nullptr);
		}

		inline void DestroyDenoiserTask(FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			auto& data = fg.GetData<DenoiserTaskData>(handle);

			rtCommandListDestroy(data.m_denoiser_command_list);

			if (!resize)
			{
				rtPostProcessingStageDestroy(data.m_denoiser_post_processor);
			}

			rtBufferDestroy(data.m_denoiser_input);
			rtBufferDestroy(data.m_denoiser_output);

			d3d12::Destroy(data.m_readback_buffer);
			SAFE_RELEASE(data.m_upload_buffer);

			if (!resize)
			{
				rtContextDestroy(data.m_optix_context);
				
				d3d12::Destroy(data.m_download_cmd_list);
				d3d12::Destroy(data.m_readback_finished_fence);
			}
		}

	} /* internal */

	template <typename T>
	inline void AddDenoiserTask(FrameGraph& frame_graph)
	{
		RenderTargetProperties rt_properties
		{
			RenderTargetProperties::IsRenderWindow(false),
			RenderTargetProperties::Width(std::nullopt),
			RenderTargetProperties::Height(std::nullopt),
			RenderTargetProperties::ExecuteResourceState(ResourceState::COPY_DEST),
			RenderTargetProperties::FinishedResourceState(ResourceState::COPY_SOURCE),
			RenderTargetProperties::CreateDSVBuffer(false),
			RenderTargetProperties::DSVFormat(Format::UNKNOWN),
			RenderTargetProperties::RTVFormats({ d3d12::settings::back_buffer_format }),
			RenderTargetProperties::NumRTVFormats(1),
			RenderTargetProperties::Clear(true),
			RenderTargetProperties::ClearDepth(false)
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::SetupDenoiserTask<T>(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle) {
			internal::ExecuteDenoiserTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::DestroyDenoiserTask(fg, handle, resize);
		};

		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COPY;
		desc.m_allow_multithreading = true;
		desc.m_post_processing = true;

		frame_graph.AddTask<DenoiserTaskData>(desc);
	}

} /* wr */

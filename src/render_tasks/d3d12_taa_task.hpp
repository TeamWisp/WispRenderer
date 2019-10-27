#pragma once
/*!
 * Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once
#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "../d3d12/d3d12_structured_buffer_pool.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../scene_graph/scene_graph.hpp"
#include "../scene_graph/camera_node.hpp"
#include "../engine_registry.hpp"

#include "../render_tasks/d3d12_deferred_main.hpp"
#include "../render_tasks/d3d12_deferred_composition.hpp"

#include <optional>

namespace wr
{
	struct TAASettings
	{
		struct Runtime
		{
			int m_quality = 16;
		};
		Runtime m_runtime;
	};

	struct TAAData
	{
		d3d12::PipelineState* m_pipeline;

		DescriptorAllocator* m_descriptor_allocator;

		DescriptorAllocation m_input_allocation;
		DescriptorAllocation m_velocity_allocation;
		DescriptorAllocation m_accum_allocation;
		DescriptorAllocation m_history_in_allocation;
		DescriptorAllocation m_history_out_allocation;
		DescriptorAllocation m_output_allocation;

		d3d12::RenderTarget* m_input;
		d3d12::RenderTarget* m_gbuffer;
		d3d12::RenderTarget* m_accum;
		d3d12::RenderTarget* m_history_in;
		d3d12::RenderTarget* m_history_out;
		d3d12::RenderTarget* m_output;

		std::vector<DirectX::XMFLOAT2> m_halton;
		int m_frame_count;
	};

	namespace internal
	{
		float VanDerCorput(int n, const int& base = 2)
		{
			float rand = 0, denom = 1, invBase = 1.f / base;
			while (n) {
				denom *= base; // 2, 4, 8, 16, etc, 2^1, 2^2, 2^3, 2^4 etc. 
				rand += (n % base) / denom;
				n *= invBase; // divide by 2 
			}
			return rand;
		}

		template<typename T>
		inline void SetupTAATask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<TAAData>(handle);
			auto& settings = fg.GetSettings<TAASettings>(handle);

			data.m_input = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<T>());
			data.m_gbuffer = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<DeferredMainTaskData>());
			data.m_output = fg.GetRenderTarget<d3d12::RenderTarget>(handle);

			d3d12::desc::RenderTargetDesc render_target_desc = {};
			render_target_desc.m_clear_color[0] = 0.f;
			render_target_desc.m_clear_color[1] = 0.f;
			render_target_desc.m_clear_color[2] = 0.f;
			render_target_desc.m_clear_color[3] = 0.f;
			render_target_desc.m_create_dsv_buffer = false;
			render_target_desc.m_dsv_format = Format::UNKNOWN;
			render_target_desc.m_initial_state = ResourceState::NON_PIXEL_SHADER_RESOURCE;
			render_target_desc.m_num_rtv_formats = 1;
			render_target_desc.m_rtv_formats[0] = data.m_input->m_create_info.m_rtv_formats[0];

			data.m_accum = d3d12::CreateRenderTarget(n_render_system.m_device,
				n_render_system.m_viewport.m_viewport.Width,
				n_render_system.m_viewport.m_viewport.Height,
				render_target_desc);

			render_target_desc.m_rtv_formats[0] = Format::R16_FLOAT;

			data.m_history_in = d3d12::CreateRenderTarget(n_render_system.m_device,
				n_render_system.m_viewport.m_viewport.Width,
				n_render_system.m_viewport.m_viewport.Height,
				render_target_desc);

			render_target_desc.m_initial_state = ResourceState::UNORDERED_ACCESS;

			data.m_history_out = d3d12::CreateRenderTarget(n_render_system.m_device,
				n_render_system.m_viewport.m_viewport.Width,
				n_render_system.m_viewport.m_viewport.Height,
				render_target_desc);

			//Retrieve the texture pool from the render system. It will be used to allocate temporary cpu visible descriptors
			std::shared_ptr<D3D12TexturePool> texture_pool = std::static_pointer_cast<D3D12TexturePool>(n_render_system.m_texture_pools[0]);
			if (!texture_pool)
			{
				LOGC("Texture pool is nullptr. This shouldn't happen as the render system should always create the first texture pool");
			}
			if (!resize)
			{
				data.m_descriptor_allocator = texture_pool->GetAllocator(DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV);

				data.m_input_allocation = std::move(data.m_descriptor_allocator->Allocate());
				data.m_velocity_allocation = std::move(data.m_descriptor_allocator->Allocate());
				data.m_accum_allocation = std::move(data.m_descriptor_allocator->Allocate());
				data.m_history_in_allocation = std::move(data.m_descriptor_allocator->Allocate());
				data.m_history_out_allocation = std::move(data.m_descriptor_allocator->Allocate());
				data.m_output_allocation = std::move(data.m_descriptor_allocator->Allocate());
			}

			{
				auto cpu_handle = data.m_input_allocation.GetDescriptorHandle();
				d3d12::CreateSRVFromSpecificRTV(data.m_input, cpu_handle, 0, data.m_input->m_create_info.m_rtv_formats[0]);
			}

			{
				auto cpu_handle = data.m_velocity_allocation.GetDescriptorHandle();
				d3d12::CreateSRVFromSpecificRTV(data.m_gbuffer, cpu_handle, 3, data.m_gbuffer->m_create_info.m_rtv_formats[3]);
			}

			{
				auto cpu_handle = data.m_accum_allocation.GetDescriptorHandle();
				d3d12::CreateSRVFromSpecificRTV(data.m_accum, cpu_handle, 0, data.m_accum->m_create_info.m_rtv_formats[0]);
			}

			{
				auto cpu_handle = data.m_history_in_allocation.GetDescriptorHandle();
				d3d12::CreateSRVFromSpecificRTV(data.m_history_in, cpu_handle, 0, data.m_history_in->m_create_info.m_rtv_formats[0]);
			}

			{
				auto cpu_handle = data.m_history_out_allocation.GetDescriptorHandle();
				d3d12::CreateUAVFromSpecificRTV(data.m_history_out, cpu_handle, 0, data.m_history_out->m_create_info.m_rtv_formats[0]);
			}

			{
				auto cpu_handle = data.m_output_allocation.GetDescriptorHandle();
				d3d12::CreateUAVFromSpecificRTV(data.m_output, cpu_handle, 0, data.m_output->m_create_info.m_rtv_formats[0]);
			}

			auto& ps_registry = PipelineRegistry::Get();
			data.m_pipeline = (d3d12::PipelineState*)ps_registry.Find(pipelines::temporal_anti_aliasing);

			data.m_halton = std::vector<DirectX::XMFLOAT2>(settings.m_runtime.m_quality);
			for (int i = 0; i < settings.m_runtime.m_quality; ++i)
			{
				DirectX::XMFLOAT2 halton;
				halton.x = (VanDerCorput(i, 2) * 2.f - 1.f) / n_render_system.m_viewport.m_viewport.Width;
				halton.y = (VanDerCorput(i, 3) * 2.f - 1.f) / n_render_system.m_viewport.m_viewport.Width;
				data.m_halton[i] = halton;
			}
		}
		
		template<typename T>
		inline void ExecuteTAATask(RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& n_device = n_render_system.m_device->m_native;
			auto& data = fg.GetData<TAAData>(handle);
			auto& settings = fg.GetSettings<TAASettings>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);
			auto frame_idx = n_render_system.GetFrameIdx();
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
			const auto viewport = n_render_system.m_viewport;

			if (settings.m_runtime.m_quality != data.m_halton.size())
			{
				data.m_halton.clear();
				data.m_halton.resize(settings.m_runtime.m_quality);

				for (int i = 0; i < settings.m_runtime.m_quality; ++i)
				{
					DirectX::XMFLOAT2 halton;
					halton.x = (VanDerCorput(i, 2) * 2.f - 1.f) / n_render_system.m_viewport.m_viewport.Width;
					halton.y = (VanDerCorput(i, 3) * 2.f - 1.f) / n_render_system.m_viewport.m_viewport.Width;
					data.m_halton[i] = halton;
				}
			}

			data.m_frame_count++;

			DirectX::XMFLOAT2 translation = data.m_halton[data.m_frame_count % settings.m_runtime.m_quality];

			DirectX::XMMATRIX dither = DirectX::XMMatrixTranslation(translation.x, translation.y, 0.f);

			sg.GetActiveCamera()->m_taa_dither = dither;

			fg.WaitForPredecessorTask<T>();

			d3d12::Transition(cmd_list, data.m_output, ResourceState::COPY_SOURCE, ResourceState::UNORDERED_ACCESS);

			d3d12::BindComputePipeline(cmd_list, data.m_pipeline);

			{
				constexpr unsigned int input = rs_layout::GetHeapLoc(params::temporal_anti_aliasing, params::TemporalAntiAliasingE::INPUT_TEXTURE);
				auto cpu_handle = data.m_input_allocation.GetDescriptorHandle();
				d3d12::SetShaderSRV(cmd_list, 0, input, cpu_handle);
			}

			{
				constexpr unsigned int velocity = rs_layout::GetHeapLoc(params::temporal_anti_aliasing, params::TemporalAntiAliasingE::MOTION_TEXTURE);
				auto cpu_handle = data.m_velocity_allocation.GetDescriptorHandle();
				d3d12::SetShaderSRV(cmd_list, 0, velocity, cpu_handle);
			}

			{
				constexpr unsigned int accum = rs_layout::GetHeapLoc(params::temporal_anti_aliasing, params::TemporalAntiAliasingE::ACCUM_TEXTURE);
				auto cpu_handle = data.m_accum_allocation.GetDescriptorHandle();
				d3d12::SetShaderSRV(cmd_list, 0, accum, cpu_handle);
			}

			{
				constexpr unsigned int hist_in = rs_layout::GetHeapLoc(params::temporal_anti_aliasing, params::TemporalAntiAliasingE::HISTORY_IN_TEXTURE);
				auto cpu_handle = data.m_history_in_allocation.GetDescriptorHandle();
				d3d12::SetShaderSRV(cmd_list, 0, hist_in, cpu_handle);
			}

			{
				constexpr unsigned int output = rs_layout::GetHeapLoc(params::temporal_anti_aliasing, params::TemporalAntiAliasingE::OUTPUT_TEXTURE);
				auto cpu_handle = data.m_output_allocation.GetDescriptorHandle();
				d3d12::SetShaderUAV(cmd_list, 0, output, cpu_handle);
			}

			{
				constexpr unsigned int hist_out = rs_layout::GetHeapLoc(params::temporal_anti_aliasing, params::TemporalAntiAliasingE::HISTORY_OUT_TEXTURE);
				auto cpu_handle = data.m_history_out_allocation.GetDescriptorHandle();
				d3d12::SetShaderUAV(cmd_list, 0, hist_out, cpu_handle);
			}

			d3d12::Dispatch(cmd_list,
				uint32_t(std::ceil(viewport.m_viewport.Width / 16.f)),
				uint32_t(std::ceil(viewport.m_viewport.Height / 16.f)),
				1);

			d3d12::Transition(cmd_list, data.m_output, ResourceState::UNORDERED_ACCESS, ResourceState::COPY_SOURCE);
			d3d12::Transition(cmd_list, data.m_history_out, ResourceState::UNORDERED_ACCESS, ResourceState::COPY_SOURCE);
			d3d12::Transition(cmd_list, data.m_accum, ResourceState::NON_PIXEL_SHADER_RESOURCE, ResourceState::COPY_DEST);
			d3d12::Transition(cmd_list, data.m_history_in, ResourceState::NON_PIXEL_SHADER_RESOURCE, ResourceState::COPY_DEST);

			cmd_list->m_native->CopyResource(data.m_accum->m_render_targets[0], data.m_output->m_render_targets[0]);
			cmd_list->m_native->CopyResource(data.m_history_in->m_render_targets[0], data.m_history_out->m_render_targets[0]);

			d3d12::Transition(cmd_list, data.m_history_out, ResourceState::COPY_SOURCE, ResourceState::UNORDERED_ACCESS);
			d3d12::Transition(cmd_list, data.m_accum, ResourceState::COPY_DEST, ResourceState::NON_PIXEL_SHADER_RESOURCE);
			d3d12::Transition(cmd_list, data.m_history_in, ResourceState::COPY_DEST, ResourceState::NON_PIXEL_SHADER_RESOURCE);
		}

		inline void DestroyTAATask(FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			auto& data = fg.GetData<TAAData>(handle);

			d3d12::Destroy(data.m_accum);
			d3d12::Destroy(data.m_history_in);
			d3d12::Destroy(data.m_history_out);

			if (!resize)
			{
				std::move(data.m_input_allocation);
				std::move(data.m_velocity_allocation);
				std::move(data.m_accum_allocation);
				std::move(data.m_history_in_allocation);
				std::move(data.m_history_out_allocation);
				std::move(data.m_output_allocation);
			}
		}
	}

	template<typename T>
	inline void AddTAATask(FrameGraph& fg)
	{
		std::wstring name(L"Temporal anti-aliasing");

		RenderTargetProperties rt_properties
		{
			RenderTargetProperties::IsRenderWindow(false),
			RenderTargetProperties::Width(std::nullopt),
			RenderTargetProperties::Height(std::nullopt),
			RenderTargetProperties::ExecuteResourceState(ResourceState::UNORDERED_ACCESS),
			RenderTargetProperties::FinishedResourceState(ResourceState::COPY_SOURCE),
			RenderTargetProperties::CreateDSVBuffer(false),
			RenderTargetProperties::DSVFormat(Format::UNKNOWN),
			RenderTargetProperties::RTVFormats({ d3d12::settings::back_buffer_format }),
			RenderTargetProperties::NumRTVFormats(1),
			RenderTargetProperties::Clear(false),
			RenderTargetProperties::ClearDepth(false),
			RenderTargetProperties::ResolutionScalar(1.0f)
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			internal::SetupTAATask<T>(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle)
		{
			internal::ExecuteTAATask<T>(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			internal::DestroyTAATask(fg, handle, resize);
		};

		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		fg.AddTask<TAAData>(desc, name, FG_DEPS<DeferredMainTaskData, T>());
		fg.UpdateSettings<TAAData>(TAASettings());
	}
}
#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "../d3d12/d3d12_structured_buffer_pool.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../scene_graph/camera_node.hpp"
#include "../scene_graph/skybox_node.hpp"
#include "../d3d12/d3d12_pipeline_registry.hpp"
#include "../engine_registry.hpp"

#include "../render_tasks/d3d12_deferred_main.hpp"
#include "../render_tasks/d3d12_cubemap_convolution.hpp"

namespace wr
{
	struct DeferredCompositionTaskData
	{
		D3D12Pipeline* in_pipeline;
		d3d12::RenderTarget* out_deferred_main_rt;

		DescriptorAllocator* out_allocator;
		DescriptorAllocation out_rtv_srv_allocation;
		DescriptorAllocation out_srv_uav_allocation;

		d3d12::TextureResource* out_skybox;
		d3d12::TextureResource* out_irradiance;

		std::array<d3d12::CommandList*, d3d12::settings::num_back_buffers> out_bundle_cmd_lists;
		bool out_requires_bundle_recording;

		bool is_in_hybrid;
	};

	namespace internal
	{

		inline void RecordDrawCommands(D3D12RenderSystem& render_system, d3d12::CommandList* cmd_list, d3d12::HeapResource* camera_cb, DeferredCompositionTaskData const & data, unsigned int frame_idx)
		{
			d3d12::BindComputePipeline(cmd_list, data.in_pipeline->m_native);

			bool is_fallback = d3d12::GetRaytracingType(render_system.m_device) == RaytracingType::FALLBACK;
			d3d12::BindDescriptorHeaps(cmd_list, frame_idx, is_fallback);

			d3d12::BindComputeConstantBuffer(cmd_list, camera_cb, 0, frame_idx);

			constexpr unsigned int albedo = rs_layout::GetHeapLoc(params::deferred_composition, params::DeferredCompositionE::GBUFFER_ALBEDO_ROUGHNESS);
			d3d12::DescHeapCPUHandle albedo_handle = data.out_rtv_srv_allocation.GetDescriptorHandle(albedo);
			d3d12::SetShaderSRV(cmd_list, 1, albedo, albedo_handle);

			constexpr unsigned int normal = rs_layout::GetHeapLoc(params::deferred_composition, params::DeferredCompositionE::GBUFFER_NORMAL_METALLIC);
			d3d12::DescHeapCPUHandle normal_handle = data.out_rtv_srv_allocation.GetDescriptorHandle(normal);
			d3d12::SetShaderSRV(cmd_list, 1, normal, normal_handle);

			constexpr unsigned int depth = rs_layout::GetHeapLoc(params::deferred_composition, params::DeferredCompositionE::GBUFFER_DEPTH);
			d3d12::DescHeapCPUHandle depth_handle = data.out_srv_uav_allocation.GetDescriptorHandle(depth);
			d3d12::SetShaderSRV(cmd_list, 1, depth, depth_handle);

			constexpr unsigned int lights = rs_layout::GetHeapLoc(params::deferred_composition, params::DeferredCompositionE::LIGHT_BUFFER);
			d3d12::DescHeapCPUHandle lights_handle = data.out_srv_uav_allocation.GetDescriptorHandle(lights);
			d3d12::SetShaderSRV(cmd_list, 1, lights, lights_handle);

			constexpr unsigned int skybox = rs_layout::GetHeapLoc(params::deferred_composition, params::DeferredCompositionE::SKY_BOX);
			d3d12::SetShaderSRV(cmd_list, 1, skybox, data.out_skybox);

			constexpr unsigned int irradiance = rs_layout::GetHeapLoc(params::deferred_composition, params::DeferredCompositionE::IRRADIANCE_MAP);
			d3d12::SetShaderSRV(cmd_list, 1, irradiance, data.out_irradiance);

			constexpr unsigned int shadow = rs_layout::GetHeapLoc(params::deferred_composition, params::DeferredCompositionE::BUFFER_REFLECTION_SHADOW);
			d3d12::DescHeapCPUHandle shadow_handle = data.out_rtv_srv_allocation.GetDescriptorHandle(shadow);
			d3d12::SetShaderSRV(cmd_list, 1, shadow, shadow_handle);

			constexpr unsigned int output = rs_layout::GetHeapLoc(params::deferred_composition, params::DeferredCompositionE::OUTPUT);
			d3d12::DescHeapCPUHandle output_handle = data.out_srv_uav_allocation.GetDescriptorHandle(output);
			d3d12::SetShaderUAV(cmd_list, 1, output, output_handle);

			d3d12::Dispatch(cmd_list,
				static_cast<int>(std::ceil(render_system.m_viewport.m_viewport.Width / 16.f)),
				static_cast<int>(std::ceil(render_system.m_viewport.m_viewport.Height / 16.f)),
				1);
		}

		template<typename T>
		inline void SetupDeferredCompositionTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<DeferredCompositionTaskData>(handle);

			auto& ps_registry = PipelineRegistry::Get();
			data.in_pipeline = (D3D12Pipeline*)ps_registry.Find(pipelines::deferred_composition);

			data.is_in_hybrid = !std::is_same<T, wr::DeferredCompositionTaskData>::value;

			//Retrieve the texture pool from the render system. It will be used to allocate temporary cpu visible descriptors
			std::shared_ptr<D3D12TexturePool> texture_pool = std::static_pointer_cast<D3D12TexturePool>(n_render_system.m_texture_pools[0]);
			if (!texture_pool)
			{
				LOGC("Texture pool is nullptr. This shouldn't happen as the render system should always create the first texture pool");
			}

			data.out_allocator = texture_pool->GetAllocator(DescriptorHeapType::DESC_HEAP_TYPE_CBV_SRV_UAV);
			data.out_rtv_srv_allocation = std::move(data.out_allocator->Allocate(9));
			data.out_srv_uav_allocation = std::move(data.out_allocator->Allocate(7));

			for (uint32_t i = 0; i < d3d12::settings::num_back_buffers; ++i)
			{
				constexpr auto rtv_id = rs_layout::GetHeapLoc(params::deferred_composition, params::DeferredCompositionE::GBUFFER_ALBEDO_ROUGHNESS);
				auto rtv_srv_handle = data.out_rtv_srv_allocation.GetDescriptorHandle(rtv_id + (2 * i));

				auto dsv_srv_handle = data.out_srv_uav_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::deferred_composition, params::DeferredCompositionE::GBUFFER_DEPTH)));

				auto deferred_main_rt = data.out_deferred_main_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<DeferredMainTaskData>());
				d3d12::CreateSRVFromRTV(deferred_main_rt, rtv_srv_handle, 2, deferred_main_rt->m_create_info.m_rtv_formats.data());
				d3d12::CreateSRVFromDSV(deferred_main_rt, dsv_srv_handle);

				if (data.is_in_hybrid)
				{
					constexpr auto shadow_id = rs_layout::GetHeapLoc(params::deferred_composition, params::DeferredCompositionE::BUFFER_REFLECTION_SHADOW);
					auto shadow_handle = data.out_rtv_srv_allocation.GetDescriptorHandle(shadow_id + i);
					
					auto hybrid_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<T>());
					d3d12::CreateSRVFromRTV(hybrid_rt, shadow_handle, 1, hybrid_rt->m_create_info.m_rtv_formats.data());
				}
			}
		}

		inline void ExecuteDeferredCompositionTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& scene_graph, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<DeferredCompositionTaskData>(handle);
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
			auto render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);

			const auto& pred_data = fg.GetPredecessorData<CubemapConvolutionTaskData>();

			if (n_render_system.m_render_window.has_value())
			{
				const auto viewport = n_render_system.m_viewport;
				const auto camera_cb = static_cast<D3D12ConstantBufferHandle*>(scene_graph.GetActiveCamera()->m_camera_cb);
				const auto frame_idx = n_render_system.GetFrameIdx();

				if (static_cast<D3D12StructuredBufferHandle*>(scene_graph.GetLightBuffer())->m_native->m_states[frame_idx] != ResourceState::NON_PIXEL_SHADER_RESOURCE)
				{
					static_cast<D3D12StructuredBufferPool*>(scene_graph.GetLightBuffer()->m_pool)->SetBufferState(scene_graph.GetLightBuffer(), ResourceState::NON_PIXEL_SHADER_RESOURCE);
					return;
				}

				//Get light buffer
				{
					auto srv_struct_buffer_handle = data.out_srv_uav_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::deferred_composition, params::DeferredCompositionE::LIGHT_BUFFER)));
					d3d12::CreateSRVFromStructuredBuffer(static_cast<D3D12StructuredBufferHandle*>(scene_graph.GetLightBuffer())->m_native, srv_struct_buffer_handle, frame_idx);
				}

				//GetSkybox
				auto skybox = scene_graph.GetCurrentSkybox();
				if (skybox != nullptr)
				{
					data.out_skybox = static_cast<wr::d3d12::TextureResource*>(scene_graph.m_skybox.value().m_pool->GetTexture(scene_graph.m_skybox.value().m_id));
					d3d12::CreateSRVFromTexture(data.out_skybox);
				}

				// Get Environment Map
				if (scene_graph.m_skybox.has_value())
				{
					data.out_irradiance = static_cast<d3d12::TextureResource*>(scene_graph.GetCurrentSkybox()->m_irradiance->m_pool->GetTexture(scene_graph.GetCurrentSkybox()->m_irradiance->m_id));
					d3d12::CreateSRVFromTexture(data.out_irradiance);
				}

				if(scene_graph.m_skybox.has_value() && !data.is_in_hybrid)
				{
					// Set skybox as hybrid output buffer?
				}

				// Output UAV
				{
					auto rtv_out_uav_handle = data.out_srv_uav_allocation.GetDescriptorHandle(COMPILATION_EVAL(rs_layout::GetHeapLoc(params::deferred_composition, params::DeferredCompositionE::OUTPUT)));
					std::vector<Format> formats = { Format::R8G8B8A8_UNORM };
					d3d12::CreateUAVFromRTV(render_target, rtv_out_uav_handle, 1, formats.data());
				}

				if constexpr (d3d12::settings::use_bundles)
				{
					// Record all bundles again if required.
					if (data.out_requires_bundle_recording)
					{
						for (auto i = 0; i < data.out_bundle_cmd_lists.size(); i++)
						{
							d3d12::Begin(data.out_bundle_cmd_lists[i], 0);
							RecordDrawCommands(n_render_system, data.out_bundle_cmd_lists[i], static_cast<D3D12ConstantBufferHandle*>(camera_cb)->m_native, data, i);
							d3d12::End(data.out_bundle_cmd_lists[i]);
						}
						data.out_requires_bundle_recording = false;
					}
				}

				//Render deferred

				d3d12::TransitionDepth(cmd_list, data.out_deferred_main_rt, ResourceState::DEPTH_WRITE, ResourceState::NON_PIXEL_SHADER_RESOURCE);

				d3d12::BindViewport(cmd_list, viewport);

				d3d12::Transition(cmd_list,
					render_target,
					wr::ResourceState::COPY_SOURCE,
					wr::ResourceState::UNORDERED_ACCESS);

				if constexpr (d3d12::settings::use_bundles)
				{
					bool is_fallback = d3d12::GetRaytracingType(n_render_system.m_device) == RaytracingType::FALLBACK;
					d3d12::BindDescriptorHeaps(cmd_list, frame_idx, is_fallback);
					d3d12::ExecuteBundle(cmd_list, data.out_bundle_cmd_lists[frame_idx]);
				}
				else
				{
					RecordDrawCommands(n_render_system, cmd_list, static_cast<D3D12ConstantBufferHandle*>(camera_cb)->m_native, data, frame_idx);
				}

				d3d12::Transition(cmd_list,
					render_target,
					wr::ResourceState::UNORDERED_ACCESS,
					wr::ResourceState::COPY_SOURCE);

				d3d12::TransitionDepth(cmd_list, data.out_deferred_main_rt, ResourceState::NON_PIXEL_SHADER_RESOURCE, ResourceState::DEPTH_WRITE);
			}
		}

		inline void DestroyDeferredCompositionTask(FrameGraph& fg, RenderTaskHandle handle)
		{
			auto& data = fg.GetData<DeferredCompositionTaskData>(handle);

			// Small hack to force the allocations to go out of scope, which will tell the texture pool to free them
			DescriptorAllocation temp1 = std::move(data.out_rtv_srv_allocation);
			DescriptorAllocation temp2 = std::move(data.out_srv_uav_allocation);
		}

	}

	template<typename T = DeferredCompositionTaskData>
	inline void AddDeferredCompositionTask(FrameGraph& fg, std::optional<unsigned int> target_width, std::optional<unsigned int> target_height, bool is_in_hybrid = false)
	{
		RenderTargetProperties rt_properties
		{
			RenderTargetProperties::IsRenderWindow(false),
			RenderTargetProperties::Width(target_width),
			RenderTargetProperties::Height(target_height),
			RenderTargetProperties::ExecuteResourceState(ResourceState::UNORDERED_ACCESS),
			RenderTargetProperties::FinishedResourceState(ResourceState::COPY_SOURCE),
			RenderTargetProperties::CreateDSVBuffer(false),
			RenderTargetProperties::DSVFormat(Format::UNKNOWN),
			RenderTargetProperties::RTVFormats({ Format::R8G8B8A8_UNORM }),
			RenderTargetProperties::NumRTVFormats(1),
			RenderTargetProperties::Clear(true),
			RenderTargetProperties::ClearDepth(true)
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool) {
			internal::SetupDeferredCompositionTask<T>(rs, fg, handle);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle) {
			internal::ExecuteDeferredCompositionTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph& fg, RenderTaskHandle handle, bool) {
			internal::DestroyDeferredCompositionTask(fg, handle);
		};
		desc.m_name = "Deferred Composition";
		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		fg.AddTask<DeferredCompositionTaskData>(desc);
	}

}
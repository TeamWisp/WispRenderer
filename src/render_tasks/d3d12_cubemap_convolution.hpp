#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_defines.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "../d3d12/d3d12_structured_buffer_pool.hpp"
#include "../d3d12/d3d12_model_pool.hpp"
#include "../d3d12/d3d12_resource_pool_texture.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../scene_graph/camera_node.hpp"
#include "../d3d12/d3d12_pipeline_registry.hpp"
#include "../engine_registry.hpp"

#include "../platform_independend_structs.hpp"
#include "d3d12_imgui_render_task.hpp"
#include "../scene_graph/camera_node.hpp"

namespace wr
{
	struct CubemapConvolutionTaskData
	{
		D3D12Pipeline* in_pipeline;

		TextureHandle in_radiance; //The skybox hdr cubemap
		TextureHandle out_irradiance;

		bool should_run = true;
	};

	namespace internal
	{
		inline void SetupCubemapFunctionalitiesTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, TextureHandle in_radiance, TextureHandle out_irradiance)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<CubemapConvolutionTaskData>(handle);

			auto& ps_registry = PipelineRegistry::Get();
			data.in_pipeline = (D3D12Pipeline*)ps_registry.Find(pipelines::equirect_to_cubemap);

			data.in_radiance = in_radiance;
			data.out_irradiance = out_irradiance;
		}

		inline void ExecuteCubemapFunctionalitiesTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& scene_graph, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<CubemapConvolutionTaskData>(handle);

			if (/*data.should_run*/true)
			{
				d3d12::TextureResource* radiance_text = static_cast<d3d12::TextureResource*>(data.in_radiance.m_pool->GetTexture(data.in_radiance.m_id));
				d3d12::TextureResource* irradiance_text = static_cast<d3d12::TextureResource*>(data.out_irradiance.m_pool->GetTexture(data.out_irradiance.m_id));




				data.should_run = false;
			}
		}

	} /* internal */

	inline void AddCubemapConvolutionTask(FrameGraph& fg, TextureHandle in_environment_map, TextureHandle out_convoluted_cubemap)
	{
		RenderTargetProperties rt_properties
		{
			false,
			std::nullopt,
			std::nullopt,
			ResourceState::RENDER_TARGET,
			ResourceState::NON_PIXEL_SHADER_RESOURCE,
			true,
			Format::D32_FLOAT,
			{ Format::R32G32B32A32_FLOAT },
			1,
			true,
			true
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [&](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool) {
			internal::SetupCubemapFunctionalitiesTask(rs, fg, handle, in_environment_map, out_convoluted_cubemap);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle) {
			internal::ExecuteCubemapFunctionalitiesTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph&, RenderTaskHandle, bool) {
			// Nothing to destroy
		};
		desc.m_name = "Cubemap Convolution";
		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = false;

		fg.AddTask<CubemapConvolutionTaskData>(desc);
	}

} /* wr */
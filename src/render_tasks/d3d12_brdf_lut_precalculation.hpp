#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_defines.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_pipeline_registry.hpp"
#include "../engine_registry.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../d3d12/d3d12_descriptors_allocations.hpp"
#include "../d3d12/d3d12_resource_pool_texture.hpp"

namespace wr
{
	struct BrdfLutTaskData
	{
		D3D12Pipeline* in_pipeline;
	};

	namespace internal
	{
		inline void SetupBrdfLutPrecalculationTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<BrdfLutTaskData>(handle);

			auto& ps_registry = PipelineRegistry::Get();
			data.in_pipeline = (D3D12Pipeline*)ps_registry.Find(pipelines::brdf_lut_precalculation);

			if (!resize && !n_render_system.m_brdf_lut.has_value())
			{
				//Retrieve the texture pool from the render system. It will be used to allocate temporary cpu visible descriptors
				std::shared_ptr<D3D12TexturePool> texture_pool = std::static_pointer_cast<D3D12TexturePool>(n_render_system.m_texture_pools[0]);
				if (!texture_pool)
				{
					LOGC("Texture pool is nullptr. This shouldn't happen as the render system should always create the first texture pool");
				}

				if (n_render_system.m_brdf_lut == std::nullopt)
				{
					n_render_system.m_brdf_lut = texture_pool->CreateTexture("BRDF LUT 2D", 512, 512, 1, wr::Format::R16G16_FLOAT, false);
				}
			}
		}

		inline void ExecuteBrdfLutPrecalculationTask(RenderSystem& rs, FrameGraph& fg, SceneGraph&, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<BrdfLutTaskData>(handle);

			if (!n_render_system.m_brdf_lut_generated)
			{
				auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);

				d3d12::BindComputePipeline(cmd_list, data.in_pipeline->m_native);

				auto* brdf_lut = static_cast<d3d12::TextureResource*>(n_render_system.m_brdf_lut.value().m_pool->GetTextureResource(n_render_system.m_brdf_lut.value()));

				d3d12::Transition(cmd_list, brdf_lut, ResourceState::COPY_DEST, ResourceState::UNORDERED_ACCESS);

				d3d12::SetShaderUAV(cmd_list, 0, COMPILATION_EVAL(rs_layout::GetHeapLoc(params::brdf_lut, params::BRDF_LutE::OUTPUT)), brdf_lut);

				d3d12::Dispatch(cmd_list, static_cast<int>(512 / 16), static_cast<int>(512 / 16.f), 1);

				d3d12::Transition(cmd_list, brdf_lut, ResourceState::UNORDERED_ACCESS, ResourceState::PIXEL_SHADER_RESOURCE);

				n_render_system.m_brdf_lut_generated = true;
				fg.SetShouldExecute(handle, false);
			}
		}
	}

	inline void AddBrdfLutPrecalculationTask(FrameGraph& fg)
	{
		std::wstring name(L"BRDF LUT Precalculation");

		RenderTargetProperties rt_properties
		{
			RenderTargetProperties::IsRenderWindow(false),
			RenderTargetProperties::Width(512),
			RenderTargetProperties::Height(512),
			RenderTargetProperties::ExecuteResourceState(ResourceState::UNORDERED_ACCESS),
			RenderTargetProperties::FinishedResourceState(ResourceState::PIXEL_SHADER_RESOURCE),
			RenderTargetProperties::CreateDSVBuffer(false),
			RenderTargetProperties::DSVFormat(Format::UNKNOWN),
			RenderTargetProperties::RTVFormats({ Format::R16G16_FLOAT }),
			RenderTargetProperties::NumRTVFormats(1),
			RenderTargetProperties::Clear(true),
			RenderTargetProperties::ClearDepth(false),
			RenderTargetProperties::ResourceName(name)
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::SetupBrdfLutPrecalculationTask(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle) {
			internal::ExecuteBrdfLutPrecalculationTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph&, RenderTaskHandle, bool) {
		};

		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		fg.AddTask<BrdfLutTaskData>(desc);
	}
}
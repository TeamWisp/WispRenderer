#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_pipeline_registry.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../d3d12/d3d12_descriptors_allocations.hpp"
#include "../d3d12/d3d12_texture_resources.hpp"

namespace wr
{
	struct DeferredCompositionTaskData
	{
		D3D12Pipeline* in_pipeline = nullptr;
		d3d12::RenderTarget* out_deferred_main_rt = nullptr;

		DescriptorAllocator* out_allocator = nullptr;

		DescriptorAllocation out_gbuffer_albedo_alloc;
		DescriptorAllocation out_gbuffer_normal_alloc;
		DescriptorAllocation out_gbuffer_emissive_alloc;
		DescriptorAllocation out_gbuffer_depth_alloc;
		DescriptorAllocation out_lights_alloc;
		DescriptorAllocation out_buffer_refl_shadow_alloc;
		DescriptorAllocation out_screen_space_irradiance_alloc;
		DescriptorAllocation out_screen_space_ao_alloc;
		DescriptorAllocation out_output_alloc;

		d3d12::TextureResource* out_skybox = nullptr;
		d3d12::TextureResource* out_irradiance = nullptr;
		d3d12::TextureResource* out_pref_env_map = nullptr;

		std::array<d3d12::CommandList*, d3d12::settings::num_back_buffers> out_bundle_cmd_lists = {};
		bool out_requires_bundle_recording = false;

		bool is_hybrid = false;
		bool is_path_tracer = false;
		bool is_rtao = false;
		bool is_hbao = false;
	};

	namespace internal
	{
		void RecordDrawCommands(D3D12RenderSystem& render_system, d3d12::CommandList* cmd_list, d3d12::HeapResource* camera_cb, DeferredCompositionTaskData const & data, unsigned int frame_idx);

		void SetupDeferredCompositionTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle);
		void ExecuteDeferredCompositionTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& scene_graph, RenderTaskHandle handle);
		void DestroyDeferredCompositionTask(FrameGraph& fg, RenderTaskHandle handle);

	}

	void AddDeferredCompositionTask(FrameGraph& fg, std::optional<unsigned int> target_width, std::optional<unsigned int> target_height);

}
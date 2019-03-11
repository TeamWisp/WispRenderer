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
		D3D12Pipeline* in_pipeline;
		d3d12::RenderTarget* out_deferred_main_rt;

		DescriptorAllocator* out_allocator;
		DescriptorAllocation out_rtv_srv_allocation;
		DescriptorAllocation out_srv_uav_allocation;

		d3d12::TextureResource* out_skybox;
		d3d12::TextureResource* out_irradiance;

		std::array<d3d12::CommandList*, d3d12::settings::num_back_buffers> out_bundle_cmd_lists;
		bool out_requires_bundle_recording;

		bool is_hybrid;
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
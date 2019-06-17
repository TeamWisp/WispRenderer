// Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../pipeline_registry.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../d3d12/d3d12_descriptors_allocations.hpp"
#include "../d3d12/d3d12_texture_resources.hpp"

namespace wr
{
	struct DeferredCompositionTaskData
	{
		d3d12::PipelineState* in_pipeline = nullptr;
		d3d12::RenderTarget* out_deferred_main_rt = nullptr;

		DescriptorAllocator* out_allocator = nullptr;

		DescriptorAllocation out_gbuffer_albedo_alloc;
		DescriptorAllocation out_gbuffer_normal_alloc;
		DescriptorAllocation out_gbuffer_emissive_alloc;
		DescriptorAllocation out_gbuffer_depth_alloc;
		DescriptorAllocation out_lights_alloc;
		DescriptorAllocation out_buffer_refl_alloc;
		DescriptorAllocation out_buffer_shadow_alloc;
		DescriptorAllocation out_screen_space_irradiance_alloc;
		DescriptorAllocation out_screen_space_ao_alloc;
		DescriptorAllocation out_output_alloc;

		d3d12::TextureResource* out_skybox = nullptr;
		d3d12::TextureResource* out_irradiance = nullptr;
		d3d12::TextureResource* out_pref_env_map = nullptr;

		std::array<d3d12::CommandList*, d3d12::settings::num_back_buffers> out_bundle_cmd_lists = {};
		bool out_requires_bundle_recording = false;

		bool is_hybrid = false;
		bool has_rt_reflection = false;
		bool has_rt_shadows = false;
		bool has_rt_shadows_denoiser = false;
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
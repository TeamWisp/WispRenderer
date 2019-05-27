#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "d3d12_deferred_main.hpp"
#include "../d3d12/d3d12_structured_buffer_pool.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../scene_graph/camera_node.hpp"
#include "../d3d12/d3d12_rt_pipeline_registry.hpp"
#include "../d3d12/d3d12_root_signature_registry.hpp"
#include "../engine_registry.hpp"

#include "../render_tasks/d3d12_deferred_main.hpp"
#include "../render_tasks/d3d12_build_acceleration_structures.hpp"
#include "../render_tasks/d3d12_cubemap_convolution.hpp"
#include "../imgui_tools.hpp"

#include <string>

namespace wr
{
	struct RTHybrid_BaseData
	{
		d3d12::AccelerationStructure out_tlas = {};

		// Shader tables
		std::array<d3d12::ShaderTable*, d3d12::settings::num_back_buffers> out_raygen_shader_table = { nullptr, nullptr, nullptr };
		std::array<d3d12::ShaderTable*, d3d12::settings::num_back_buffers> out_miss_shader_table = { nullptr, nullptr, nullptr };
		std::array<d3d12::ShaderTable*, d3d12::settings::num_back_buffers> out_hitgroup_shader_table = { nullptr, nullptr, nullptr };


		// Pipeline objects
		d3d12::StateObject* out_state_object = nullptr;
		d3d12::RootSignature* out_root_signature = nullptr;

		// Structures and buffers
		D3D12ConstantBufferHandle* out_cb_camera_handle = nullptr;
		d3d12::RenderTarget* out_deferred_main_rt = nullptr;

		unsigned int frame_idx = 0;

		DescriptorAllocation out_output_alloc;
		DescriptorAllocation out_gbuffer_albedo_alloc;
		DescriptorAllocation out_gbuffer_normal_alloc;
		DescriptorAllocation out_gbuffer_depth_alloc;

		bool tlas_requires_init = false;
	};

	namespace internal
	{
		inline void CreateUAVsAndSRVs(FrameGraph& fg, RTHybrid_BaseData& data, d3d12::RenderTarget* n_render_target)
		{
			// Versioning
			for (int frame_idx = 0; frame_idx < 1; ++frame_idx)
			{
				// Bind output texture
				d3d12::DescHeapCPUHandle rtv_handle = data.out_output_alloc.GetDescriptorHandle();
				d3d12::CreateUAVFromSpecificRTV(n_render_target, rtv_handle, frame_idx, n_render_target->m_create_info.m_rtv_formats[frame_idx]);

				// Bind g-buffers (albedo, normal, depth)
				auto albedo_handle = data.out_gbuffer_albedo_alloc.GetDescriptorHandle();
				auto normal_handle = data.out_gbuffer_normal_alloc.GetDescriptorHandle();
				auto depth_handle = data.out_gbuffer_depth_alloc.GetDescriptorHandle();

				auto deferred_main_rt = data.out_deferred_main_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<DeferredMainTaskData>());

				d3d12::CreateSRVFromSpecificRTV(deferred_main_rt, albedo_handle, 0, deferred_main_rt->m_create_info.m_rtv_formats[0]);
				d3d12::CreateSRVFromSpecificRTV(deferred_main_rt, normal_handle, 1, deferred_main_rt->m_create_info.m_rtv_formats[1]);

				d3d12::CreateSRVFromDSV(deferred_main_rt, depth_handle);
			}
		}
	} /* internal */
} /* wr */
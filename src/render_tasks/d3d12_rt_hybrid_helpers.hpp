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
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "d3d12_deferred_main.hpp"
#include "../d3d12/d3d12_structured_buffer_pool.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../scene_graph/camera_node.hpp"
#include "../rt_pipeline_registry.hpp"
#include "../root_signature_registry.hpp"
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
		inline void CreateShaderTables(d3d12::Device* device, RTHybrid_BaseData& data, const std::string& raygen_entry, 
									   const std::vector<std::string>& miss_entries, const std::vector<std::string>& hit_groups, int frame_idx)
		{
			// Delete existing shader table
			if (data.out_miss_shader_table[frame_idx])
			{
				d3d12::Destroy(data.out_miss_shader_table[frame_idx]);
			}
			if (data.out_hitgroup_shader_table[frame_idx])
			{
				d3d12::Destroy(data.out_hitgroup_shader_table[frame_idx]);
			}
			if (data.out_raygen_shader_table[frame_idx])
			{
				d3d12::Destroy(data.out_raygen_shader_table[frame_idx]);
			}

			// Set up Raygen Shader Table
			{
				// Create Record(s)
				std::uint32_t shader_record_count = 1;
				auto shader_identifier_size = d3d12::GetShaderIdentifierSize(device);
				auto shader_identifier = d3d12::GetShaderIdentifier(device, data.out_state_object, raygen_entry);

				auto shader_record = d3d12::CreateShaderRecord(shader_identifier, shader_identifier_size);

				// Create Table
				data.out_raygen_shader_table[frame_idx] = d3d12::CreateShaderTable(device, shader_record_count, shader_identifier_size);
				d3d12::AddShaderRecord(data.out_raygen_shader_table[frame_idx], shader_record);
			}

			// Set up Miss Shader Table
			{
				// Create Record(s) and Table(s)
				std::uint32_t shader_record_count = miss_entries.size();
				auto shader_identifier_size = d3d12::GetShaderIdentifierSize(device);

				data.out_miss_shader_table[frame_idx] = d3d12::CreateShaderTable(device, shader_record_count, shader_identifier_size);
				
				for (auto& entry : miss_entries)
				{
					auto miss_identifier = d3d12::GetShaderIdentifier(device, data.out_state_object, entry);
					auto miss_record = d3d12::CreateShaderRecord(miss_identifier, shader_identifier_size);
					d3d12::AddShaderRecord(data.out_miss_shader_table[frame_idx], miss_record);
				}
			}

			// Set up Hit Group Shader Table
			{
				// Create Record(s)
				std::uint32_t shader_record_count = hit_groups.size();
				auto shader_identifier_size = d3d12::GetShaderIdentifierSize(device);

				data.out_hitgroup_shader_table[frame_idx] = d3d12::CreateShaderTable(device, shader_record_count, shader_identifier_size);
				
				for (auto& entry : hit_groups)
				{
					auto hit_identifier = d3d12::GetShaderIdentifier(device, data.out_state_object, entry);
					auto hit_record = d3d12::CreateShaderRecord(hit_identifier, shader_identifier_size);
					d3d12::AddShaderRecord(data.out_hitgroup_shader_table[frame_idx], hit_record);
				}
			}
		}


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
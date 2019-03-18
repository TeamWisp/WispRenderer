#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "../d3d12/d3d12_structured_buffer_pool.hpp"
#include "../d3d12/d3d12_rt_descriptor_heap.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../engine_registry.hpp"
#include "../util/math.hpp"

#include "../render_tasks/d3d12_deferred_main.hpp"
#include "../imgui_tools.hpp"

namespace wr
{
	struct ASBuildData
	{
		DescriptorAllocator* out_allocator;
		DescriptorAllocation out_scene_ib_alloc;
		DescriptorAllocation out_scene_mat_alloc;
		DescriptorAllocation out_scene_offset_alloc;

		d3d12::AccelerationStructure out_tlas;
		D3D12StructuredBufferHandle* out_sb_material_handle;
		D3D12StructuredBufferHandle* out_sb_offset_handle;
		std::vector<std::tuple<d3d12::AccelerationStructure, unsigned int, DirectX::XMMATRIX>> out_blas_list;
		std::vector<temp::RayTracingMaterial_CBData> out_materials;
		std::vector<temp::RayTracingOffset_CBData> out_offsets;
		std::unordered_map<unsigned int, unsigned int> out_parsed_materials;
		std::vector<MaterialHandle*> out_material_handles;
		d3d12::StagingBuffer* out_scene_ib;
		d3d12::StagingBuffer* out_scene_vb;

		std::unordered_map<std::uint64_t, d3d12::AccelerationStructure> blasses;

		bool out_init;
		bool out_materials_require_update;
	};

	namespace internal
	{

		inline void SetupBuildASTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			if (resize) return;

			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& device = n_render_system.m_device;
			auto& data = fg.GetData<ASBuildData>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);
			d3d12::SetName(n_render_target, L"Raytracing Target");

			data.out_init = true;
			data.out_materials_require_update = true;


		inline void ExecuteBuildASTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& scene_graph, RenderTaskHandle handle)
		{
			auto& data = fg.GetData<ASBuildData>(handle);
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto device = n_render_system.m_device;

			data.out_materials_require_update = false;

			d3d12::DescriptorHeap* out_heap = cmd_list->m_rt_descriptor_heap->GetHeap();

			// Initialize requirements
			if (data.out_init)
			{
				std::vector<std::shared_ptr<D3D12ModelPool>> model_pools = n_render_system.m_model_pools;
				// Transition all model pools for accel structure creation
				for (auto& pool : model_pools)
				{
					d3d12::Transition(cmd_list, pool->GetVertexStagingBuffer(), ResourceState::VERTEX_AND_CONSTANT_BUFFER, ResourceState::NON_PIXEL_SHADER_RESOURCE);
					d3d12::Transition(cmd_list, pool->GetIndexStagingBuffer(), ResourceState::INDEX_BUFFER, ResourceState::NON_PIXEL_SHADER_RESOURCE);
				}

				// List all materials used by meshes
				internal::BuildBLASList(device, cmd_list, scene_graph, data);

				data.out_tlas = d3d12::CreateTopLevelAccelerationStructure(device, cmd_list, out_heap, data.out_blas_list);
				data.out_tlas.m_native->SetName(L"Highlevelaccel");

				// Transition all model pools back to whatever they were.
				for (auto& pool : model_pools)
				{
					d3d12::Transition(cmd_list, pool->GetVertexStagingBuffer(), ResourceState::NON_PIXEL_SHADER_RESOURCE, ResourceState::VERTEX_AND_CONSTANT_BUFFER);
					d3d12::Transition(cmd_list, pool->GetIndexStagingBuffer(), ResourceState::NON_PIXEL_SHADER_RESOURCE, ResourceState::INDEX_BUFFER);
				}

				internal::CreateSRVs(cmd_list, data);

				data.out_init = false;
			}
			else
			{
				internal::UpdateTLAS(device, cmd_list, scene_graph, data);
			}
		}

		inline void DestroyBuildASTask(FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
			if (!resize)
			{
				auto& data = fg.GetData<ASBuildData>(handle);

				// Small hack to force the allocations to go out of scope, which will tell the allocator to free them
				DescriptorAllocation temp1 = std::move(data.out_scene_ib_alloc);
				DescriptorAllocation temp2 = std::move(data.out_scene_mat_alloc);
				DescriptorAllocation temp3 = std::move(data.out_scene_offset_alloc);
			}
		}

	} /* internal */

	inline void AddBuildAccelerationStructuresTask(FrameGraph& frame_graph)
	{
		RenderTargetProperties rt_properties
		{
			RenderTargetProperties::IsRenderWindow(false),
			RenderTargetProperties::Width(std::nullopt),
			RenderTargetProperties::Height(std::nullopt),
			RenderTargetProperties::ExecuteResourceState(ResourceState::UNORDERED_ACCESS),
			RenderTargetProperties::FinishedResourceState(ResourceState::COPY_SOURCE),
			RenderTargetProperties::CreateDSVBuffer(false),
			RenderTargetProperties::DSVFormat(Format::UNKNOWN),
			RenderTargetProperties::RTVFormats({ Format::UNKNOWN }),
			RenderTargetProperties::NumRTVFormats(0),
			RenderTargetProperties::Clear(false),
			RenderTargetProperties::ClearDepth(false)
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::SetupBuildASTask(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle) {
			internal::ExecuteBuildASTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::DestroyBuildASTask(fg, handle, resize);
		};
		desc.m_name = "Acceleration Structure Builder";
		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		frame_graph.AddTask<ASBuildData>(desc);
	}

} /* wr */

#include "d3d12_functions.hpp"

#include <variant>

#include "d3d12_defines.hpp"
#include "d3d12_rt_descriptor_heap.hpp"

namespace wr::d3d12
{

	/*!
	NOTE!
	Top level has scratch and instance descs.
	blas does not!!!!!!!!
	*/

	namespace internal
	{

		inline void AllocateUAVBuffer(Device* device, UINT64 size, ID3D12Resource** resource, D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_COMMON, const wchar_t* name = nullptr)
		{
			auto heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			auto buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
			TRY(device->m_native->CreateCommittedResource(
				&heap_properties,
				D3D12_HEAP_FLAG_NONE,
				&buffer_desc,
				initial_state,
				nullptr,
				IID_PPV_ARGS(resource)));
			if (name)
			{
				NAME_D3D12RESOURCE((*resource), name);
			}
		}

		inline void AllocateUploadBuffer(Device* device, void* data, UINT64 size, ID3D12Resource** resource, const wchar_t* name = nullptr)
		{
			auto heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			auto buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(size);
			TRY(device->m_native->CreateCommittedResource(
				&heap_properties,
				D3D12_HEAP_FLAG_NONE,
				&buffer_desc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(resource)));
			if (name)
			{
				NAME_D3D12RESOURCE((*resource), name);
			}

			void *mapped_data;
			(*resource)->Map(0, nullptr, &mapped_data);
			memcpy(mapped_data, data, size);
			(*resource)->Unmap(0, nullptr);
		}

		inline void UpdateUploadbuffer(void* data, UINT64 size, ID3D12Resource* resource)
		{
			void *mapped_data;
			resource->Map(0, nullptr, &mapped_data);
			memcpy(mapped_data, data, size);
			resource->Unmap(0, nullptr);
		}

		WRAPPED_GPU_POINTER CreateFallbackWrappedPointer(
			Device* device,
			DescriptorHeap* heap,
			std::uint32_t index,
			ID3D12Resource* resource,
			UINT buffer_num_elements)
		{

			if (GetRaytracingType(device) != RaytracingType::FALLBACK)
			{
				LOGW("CreateFallbackWrappedPointer got called but the device isn't setup for fallback.");
			}

			D3D12_UNORDERED_ACCESS_VIEW_DESC rawBufferUavDesc = {};
			rawBufferUavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			rawBufferUavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
			rawBufferUavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			rawBufferUavDesc.Buffer.NumElements = buffer_num_elements;

			d3d12::DescHeapCPUHandle bottom_level_descriptor;

			// Only compute fallback requires a valid descriptor index when creating a wrapped pointer.
			UINT desc_heap_idx = index; // TODO don't hardcode this.
			if (!device->m_fallback_native->UsingRaytracingDriver())
			{
				for (auto frame_idx = 0; frame_idx < 3; frame_idx++)
				{
					bottom_level_descriptor = d3d12::GetCPUHandle(heap, frame_idx, 0); // TODO: Don't harcode this.
					d3d12::Offset(bottom_level_descriptor, desc_heap_idx, heap->m_increment_size);
					device->m_native->CreateUnorderedAccessView(resource, nullptr, &rawBufferUavDesc, bottom_level_descriptor.m_native);
				}
			}
			return device->m_fallback_native->GetWrappedPointerSimple(desc_heap_idx, resource->GetGPUVirtualAddress());
		}

		inline void UpdatePrebuildInfo(Device* device, AccelerationStructure& as, D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs)
		{
			if (GetRaytracingType(device) == RaytracingType::NATIVE)
			{
				device->m_native->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &as.m_prebuild_info);
			}
			else if (GetRaytracingType(device) == RaytracingType::FALLBACK)
			{
				device->m_fallback_native->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &as.m_prebuild_info);
			}
			if (!(as.m_prebuild_info.ResultDataMaxSizeInBytes > 0)) LOGW("Result data max size in bytes is more than zero. accel structure");
		}

		inline void BuildAS(Device* device, CommandList* cmd_list, DescriptorHeap* desc_heap, AccelerationStructure& as, D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC const & desc)
		{
			auto BuildAccelerationStructure = [&](auto * raytracingCommandList)
			{
				raytracingCommandList->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);
			};

			// Build acceleration structure.
			if (GetRaytracingType(device) == RaytracingType::NATIVE)
			{
				BuildAccelerationStructure(cmd_list->m_native);
			}
			else if (GetRaytracingType(device) == RaytracingType::FALLBACK)
			{
				// Set the descriptor heaps to be used during acceleration structure build for the Fallback Layer.
				d3d12::BindDescriptorHeap(cmd_list, desc_heap, desc_heap->m_create_info.m_type, 0, true); //TODO: note this non frame idx
				BuildAccelerationStructure(cmd_list->m_native_fallback);
			}
		}

		inline void CreateInstancesForTLAS(Device* device, AccelerationStructure& tlas, DescriptorHeap* desc_heap, std::vector<desc::BlasDesc> blas_list, std::uint32_t frame_idx, bool update)
		{
			// Falback layer heap offset
			auto fallback_heap_idx = d3d12::settings::fallback_ptrs_offset;

			// Create the instances to the bottom level instances.
			if (GetRaytracingType(device) == RaytracingType::NATIVE)
			{
				std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instance_descs;
				for (auto it : blas_list)
				{
					auto blas = it.m_as;
					auto material = it.m_material;
					auto transform = it.m_transform;

					D3D12_RAYTRACING_INSTANCE_DESC instance_desc = {};

					XMStoreFloat3x4(reinterpret_cast<DirectX::XMFLOAT3X4*>(instance_desc.Transform), transform);

					instance_desc.InstanceMask = 1;
					instance_desc.InstanceID = material;
					instance_desc.AccelerationStructure = blas.m_natives[frame_idx]->GetGPUVirtualAddress();
					instance_descs.push_back(instance_desc);
				}

				if (update)
				{
					internal::UpdateUploadbuffer(instance_descs.data(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * blas_list.size(), tlas.m_instance_descs[frame_idx]);
				}
				else
				{
					for (auto& inst_desc : tlas.m_instance_descs)
					{
						internal::AllocateUploadBuffer(device, instance_descs.data(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * blas_list.size(), &inst_desc, L"InstanceDescs");
					}
				}
			}
			else if (GetRaytracingType(device) == RaytracingType::FALLBACK)
			{
				std::vector<D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC> instance_descs;
				for (auto it : blas_list)
				{
					auto blas = it.m_as;
					auto material = it.m_material;
					auto transform = it.m_transform;

					D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC instance_desc = {};

					XMStoreFloat3x4(reinterpret_cast<DirectX::XMFLOAT3X4*>(instance_desc.Transform), transform);

					instance_desc.InstanceMask = 1;
					instance_desc.InstanceID = material;
					std::uint32_t num_buffer_elements = static_cast<std::uint32_t>(blas.m_prebuild_info.ResultDataMaxSizeInBytes) / sizeof(std::uint32_t);
					instance_desc.AccelerationStructure = internal::CreateFallbackWrappedPointer(device, desc_heap, fallback_heap_idx, blas.m_natives[frame_idx], num_buffer_elements);

					instance_descs.push_back(instance_desc);

					fallback_heap_idx++;
				}

				if (update)
				{
					internal::UpdateUploadbuffer(instance_descs.data(), sizeof(D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC) * instance_descs.size(), tlas.m_instance_descs[frame_idx]);
				}
				else
				{
					for (auto& inst_desc : tlas.m_instance_descs)
					{
						internal::AllocateUploadBuffer(device, instance_descs.data(), sizeof(D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC) * instance_descs.size(), &inst_desc, L"InstanceDescs");
					}
				}
			}

			// Create a wrapped pointer to the acceleration structure.
			if (GetRaytracingType(device) == RaytracingType::FALLBACK)
			{
				std::uint32_t num_buffer_elements = static_cast<std::uint32_t>(tlas.m_prebuild_info.ResultDataMaxSizeInBytes) / sizeof(std::uint32_t);
				tlas.m_fallback_tlas_ptr = internal::CreateFallbackWrappedPointer(device, desc_heap, fallback_heap_idx, tlas.m_natives[0], num_buffer_elements);
			}
		}

		inline void CopyInstDescResource(CommandList* cmd_list, AccelerationStructure& as, std::uint32_t source_index, std::uint32_t target_index)
		{
			cmd_list->m_native->CopyResource(as.m_instance_descs[target_index], as.m_instance_descs[source_index]);
		}

		inline void CopyAS(Device* device, CommandList* cmd_list, AccelerationStructure& as, std::uint32_t source_index, std::uint32_t target_index)
		{
			if (GetRaytracingType(device) == RaytracingType::NATIVE)
			{
				cmd_list->m_native->CopyRaytracingAccelerationStructure(as.m_natives[target_index]->GetGPUVirtualAddress(), as.m_natives[source_index]->GetGPUVirtualAddress(), D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_CLONE);
			}
			else if (GetRaytracingType(device) == RaytracingType::FALLBACK)
			{
				cmd_list->m_native_fallback->CopyRaytracingAccelerationStructure(as.m_natives[target_index]->GetGPUVirtualAddress(), as.m_natives[source_index]->GetGPUVirtualAddress(), D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_CLONE);
			}
		}

	} /* internal */


#include <DirectXMath.h>

	[[nodiscard]] AccelerationStructure CreateBottomLevelAccelerationStructures(Device* device,
		CommandList* cmd_list,
		DescriptorHeap* desc_heap,
		std::vector<desc::GeometryDesc> geometry)
	{
		AccelerationStructure blas = {};

		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometry_descs(geometry.size());
		for (auto i = 0; i < geometry.size(); i++)
		{
			auto geom = geometry[i];

			geometry_descs[i].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
			if (auto index_buffer = geom.index_buffer.value_or(nullptr))
			{
				geometry_descs[i].Triangles.IndexBuffer = index_buffer->m_buffer->GetGPUVirtualAddress() + (geom.m_indices_offset * index_buffer->m_stride_in_bytes);
				geometry_descs[i].Triangles.IndexCount = geom.m_num_indices;
				geometry_descs[i].Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
				geometry_descs[i].Triangles.Transform3x4 = 0;
			}
			else
			{
				geometry_descs[i].Triangles.IndexBuffer = 0;
				geometry_descs[i].Triangles.IndexCount = 0;
				geometry_descs[i].Triangles.IndexFormat = DXGI_FORMAT_UNKNOWN;
				geometry_descs[i].Triangles.Transform3x4 = 0;
			}
			geometry_descs[i].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
			geometry_descs[i].Triangles.VertexCount = geom.m_num_vertices;
			geometry_descs[i].Triangles.VertexBuffer.StartAddress = geom.vertex_buffer->m_buffer->GetGPUVirtualAddress() + (geom.m_vertices_offset * geom.m_vertex_stride);
			geometry_descs[i].Triangles.VertexBuffer.StrideInBytes = geom.m_vertex_stride;
			geometry_descs[i].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
			//geometry_descs[i].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
			// AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
		}

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS build_flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

		// Get prebuild info bottom level
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottom_level_inputs;
		bottom_level_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		bottom_level_inputs.NumDescs = static_cast<std::uint32_t>(geometry.size());
		bottom_level_inputs.pGeometryDescs = geometry_descs.data();
		bottom_level_inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		bottom_level_inputs.Flags = build_flags;

		// Get bottom level prebuild info
		internal::UpdatePrebuildInfo(device, blas, bottom_level_inputs);

		// Allocate scratch resource
		internal::AllocateUAVBuffer(device,
			blas.m_prebuild_info.ScratchDataSizeInBytes,
			&blas.m_scratch,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			L"Acceleration Structure Scratch Resource");

		// Allocate resources for acceleration structures.
		for (auto& as : blas.m_natives) {
			D3D12_RESOURCE_STATES initial_resource_state = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
			if (GetRaytracingType(device) == RaytracingType::FALLBACK)
			{
				initial_resource_state = device->m_fallback_native->GetAccelerationStructureResourceState();
			}

			internal::AllocateUAVBuffer(device, blas.m_prebuild_info.ResultDataMaxSizeInBytes, &as, initial_resource_state, L"BottomLevelAccelerationStructure");
		}

		// Bottom Level Acceleration Structure desc
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottom_level_build_desc = {};
		{
			bottom_level_build_desc.Inputs = bottom_level_inputs;
			bottom_level_build_desc.ScratchAccelerationStructureData = blas.m_scratch->GetGPUVirtualAddress();
			bottom_level_build_desc.DestAccelerationStructureData = blas.m_natives[0]->GetGPUVirtualAddress();
		}

		internal::BuildAS(device, cmd_list, desc_heap, blas, bottom_level_build_desc);
		d3d12::UAVBarrierAS(cmd_list, blas, 0);

		for (std::uint8_t i = 1; i < settings::num_back_buffers; i++)
		{
			internal::CopyAS(device, cmd_list, blas, 0, i);
		}

		return blas;
	}

	AccelerationStructure CreateTopLevelAccelerationStructure(Device* device,
		CommandList* cmd_list,
		DescriptorHeap* desc_heap,
		std::vector<desc::BlasDesc> blas_list)
	{
		AccelerationStructure tlas = {};

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS build_flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS top_level_inputs;
		top_level_inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		top_level_inputs.Flags = build_flags;
		top_level_inputs.NumDescs = static_cast<std::uint32_t>(blas_list.size());
		top_level_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

		// Get prebuild info top level
		internal::UpdatePrebuildInfo(device, tlas, top_level_inputs);

		// Allocate scratch resource
		internal::AllocateUAVBuffer(device,
			tlas.m_prebuild_info.ScratchDataSizeInBytes,
			&tlas.m_scratch,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			L"Acceleration Structure Scratch Resource");


		// Allocate acceleration structure buffer
		for (auto& as : tlas.m_natives) {
			D3D12_RESOURCE_STATES initial_resoruce_state = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
			if (GetRaytracingType(device) == RaytracingType::FALLBACK)
			{
				initial_resoruce_state = device->m_fallback_native->GetAccelerationStructureResourceState();
			}

			internal::AllocateUAVBuffer(device, tlas.m_prebuild_info.ResultDataMaxSizeInBytes, &as, initial_resoruce_state, L"TopLevelAccelerationStructure");
		}

		// Create the instances to the bottom level instances.
		internal::CreateInstancesForTLAS(device, tlas, desc_heap, blas_list, 0, false);

		// Top Level Acceleration Structure desc
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC top_level_build_desc = {};
		{
			top_level_inputs.InstanceDescs = tlas.m_instance_descs[0]->GetGPUVirtualAddress();
			top_level_build_desc.Inputs = top_level_inputs;
			top_level_build_desc.DestAccelerationStructureData = tlas.m_natives[0]->GetGPUVirtualAddress();
			top_level_build_desc.ScratchAccelerationStructureData = tlas.m_scratch->GetGPUVirtualAddress();
		}

		internal::BuildAS(device, cmd_list, desc_heap, tlas, top_level_build_desc);

		d3d12::UAVBarrierAS(cmd_list, tlas, 0);
		for (std::uint8_t i = 1; i < settings::num_back_buffers; i++)
		{
			internal::CopyAS(device, cmd_list, tlas, 0, i);
		}

		return tlas;
	}

	void DestroyAccelerationStructure(AccelerationStructure& structure)
	{
		SAFE_RELEASE(structure.m_scratch);

		for (auto& as : structure.m_natives)
		{
			SAFE_RELEASE(as);
		}
		for (auto& inst_desc : structure.m_instance_descs)
		{
			SAFE_RELEASE(inst_desc);
		}
	}

	void UAVBarrierAS(CommandList* cmd_list, AccelerationStructure const & structure, std::uint32_t frame_idx)
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(structure.m_natives[frame_idx]);
		cmd_list->m_native->ResourceBarrier(1, &barrier);
	}

	void UpdateTopLevelAccelerationStructure(AccelerationStructure& tlas, Device* device,
		CommandList* cmd_list,
		DescriptorHeap* desc_heap,
		std::vector<desc::BlasDesc> blas_list,
		std::uint32_t frame_idx)
	{
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS build_flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS top_level_inputs;
		top_level_inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		top_level_inputs.Flags = build_flags;
		top_level_inputs.NumDescs = static_cast<std::uint32_t>(blas_list.size());
		top_level_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO old_prebuild_info = tlas.m_prebuild_info;

		internal::UpdatePrebuildInfo(device, tlas, top_level_inputs);

		top_level_inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;

		bool rebuild_accel_structure = old_prebuild_info.ResultDataMaxSizeInBytes < tlas.m_prebuild_info.ResultDataMaxSizeInBytes ||
			old_prebuild_info.ScratchDataSizeInBytes < tlas.m_prebuild_info.ScratchDataSizeInBytes ||
			old_prebuild_info.UpdateScratchDataSizeInBytes < tlas.m_prebuild_info.UpdateScratchDataSizeInBytes;

		if (rebuild_accel_structure)
		{
			LOGW("Complete AS rebuild triggered. This might break versioining");
			tlas = CreateTopLevelAccelerationStructure(device, cmd_list, desc_heap, blas_list);
		}
		else
		{
			// Create the instances to the bottom level instances.
			internal::CreateInstancesForTLAS(device, tlas, desc_heap, blas_list, frame_idx, true);

			// Top Level Acceleration Structure desc
			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC top_level_build_desc = {};
			{
				auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(tlas.m_natives[frame_idx]);
				cmd_list->m_native->ResourceBarrier(1, &barrier);

				top_level_inputs.InstanceDescs = tlas.m_instance_descs[frame_idx]->GetGPUVirtualAddress();
				top_level_build_desc.Inputs = top_level_inputs;
				top_level_build_desc.SourceAccelerationStructureData = tlas.m_natives[frame_idx]->GetGPUVirtualAddress(); //TODO: Benchmark performance when taking the previous source.
				top_level_build_desc.DestAccelerationStructureData = tlas.m_natives[frame_idx]->GetGPUVirtualAddress();
				top_level_build_desc.ScratchAccelerationStructureData = tlas.m_scratch->GetGPUVirtualAddress();
			}

			internal::BuildAS(device, cmd_list, desc_heap, tlas, top_level_build_desc);
		}
	}

	void SetName(AccelerationStructure& acceleration_structure, std::wstring name)
	{
		for (auto& as : acceleration_structure.m_natives)
		{
			as->SetName((name + L" - Acceleration Structure").c_str());
		}
		for (auto& inst_desc : acceleration_structure.m_instance_descs)
		{
			if (inst_desc)
			{
				inst_desc->SetName((name + L" - Acceleration Structure").c_str());
			}
		}
	}

	void CreateOrUpdateTLAS(Device* device, CommandList* cmd_list, bool& requires_init, d3d12::AccelerationStructure& out_tlas,
		std::vector<desc::BlasDesc> blas_list, std::uint32_t frame_idx)
	{
		d3d12::DescriptorHeap* heap = static_cast<d3d12::CommandList*>(cmd_list)->m_rt_descriptor_heap->GetHeap();

		if (d3d12::GetRaytracingType(device) == RaytracingType::FALLBACK)
		{
			if (requires_init)
			{
				out_tlas = d3d12::CreateTopLevelAccelerationStructure(device, cmd_list, heap, blas_list);

				requires_init = false;
			}
			else
			{
				d3d12::UpdateTopLevelAccelerationStructure(out_tlas, device, cmd_list, heap, blas_list, frame_idx);
			}
		}
	}

} /* wr::d3d12 */
#include "d3d12_functions.hpp"

#include <variant>

#include "d3d12_defines.hpp"

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
			UINT desc_heap_idx = 0; // TODO don't hardcode this.
			if (!device->m_fallback_native->UsingRaytracingDriver())
			{
				// desc_heap_idx = AllocateDescriptor(heap, increment_size, &bottomLevelDescriptor, index);
				bottom_level_descriptor = d3d12::GetCPUHandle(heap, 0, desc_heap_idx); // TODO: Don't harcode this.
				device->m_native->CreateUnorderedAccessView(resource, nullptr, &rawBufferUavDesc, bottom_level_descriptor.m_native);
			}
			return device->m_fallback_native->GetWrappedPointerSimple(desc_heap_idx, resource->GetGPUVirtualAddress());
		}

	} /* internal */

	[[nodiscard]] std::pair<AccelerationStructure, AccelerationStructure> CreateAccelerationStructures(Device* device,
		CommandList* cmd_list,
		DescriptorHeap* desc_heap,
		std::vector<StagingBuffer*> vertex_buffers)
	{
		AccelerationStructure blas;
		AccelerationStructure tlas;

		D3D12_RAYTRACING_GEOMETRY_DESC geometry_desc = {};
		geometry_desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geometry_desc.Triangles.IndexBuffer = 0;
		geometry_desc.Triangles.IndexCount = 0;
		geometry_desc.Triangles.IndexFormat = DXGI_FORMAT_UNKNOWN;
		geometry_desc.Triangles.Transform3x4 = 0;
		geometry_desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		geometry_desc.Triangles.VertexCount = 3;
		geometry_desc.Triangles.VertexBuffer.StartAddress = vertex_buffers[0]->m_staging->GetGPUVirtualAddress();
		geometry_desc.Triangles.VertexBuffer.StrideInBytes = sizeof(vertex_buffers[0]->m_stride_in_bytes);

		// Mark the geometry as opaque. 
		// PERFORMANCE TIP: mark geometry as opaque whenever applicable as it can enable important ray processing optimizations.
		// Note: When rays encounter opaque geometry an any hit shader will not be executed whether it is present or not.
		geometry_desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS build_flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC top_level_build_desc = {};
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& top_level_prebuild_info_desc = top_level_build_desc.Inputs;
		top_level_prebuild_info_desc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		top_level_prebuild_info_desc.Flags = build_flags;
		top_level_prebuild_info_desc.NumDescs = 1;
		top_level_prebuild_info_desc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		top_level_prebuild_info_desc.pGeometryDescs = nullptr;

		// Get prebuild info top level
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO top_level_prebuild_info = {};
		if (GetRaytracingType(device) == RaytracingType::NATIVE)
		{
			device->m_native->GetRaytracingAccelerationStructurePrebuildInfo(&top_level_prebuild_info_desc, &top_level_prebuild_info);
		}
		else if (GetRaytracingType(device) == RaytracingType::FALLBACK)
		{
			device->m_fallback_native->GetRaytracingAccelerationStructurePrebuildInfo(&top_level_prebuild_info_desc, &top_level_prebuild_info);
		}

		// Get prebuild info bottom level
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottom_level_build_desc = {};
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& bottom_level_prebuild_info_desc = bottom_level_build_desc.Inputs;
		bottom_level_prebuild_info_desc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		bottom_level_prebuild_info_desc.Flags = build_flags;
		bottom_level_prebuild_info_desc.NumDescs = 1;
		bottom_level_prebuild_info_desc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		bottom_level_prebuild_info_desc.pGeometryDescs = &geometry_desc;
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottom_level_prebuild_info = {};
		if (GetRaytracingType(device) == RaytracingType::NATIVE)
		{
			device->m_native->GetRaytracingAccelerationStructurePrebuildInfo(&bottom_level_prebuild_info_desc, &bottom_level_prebuild_info);
		}
		else if (GetRaytracingType(device) == RaytracingType::FALLBACK)
		{
			device->m_fallback_native->GetRaytracingAccelerationStructurePrebuildInfo(&bottom_level_prebuild_info_desc, &bottom_level_prebuild_info);
		}

		// Allocate scratch resource
		internal::AllocateUAVBuffer(device,
			max(top_level_prebuild_info.ScratchDataSizeInBytes, bottom_level_prebuild_info.ScratchDataSizeInBytes),
			&tlas.m_scratch,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			L"Acceleration Structure Scratch Resource");

		// Allocate resources for acceleration structures.
		// Acceleration structures can only be placed in resources that are created in the default heap (or custom heap equivalent). 
		// Default heap is OK since the application doesnt need CPU read/write access to them. 
		// The resources that will contain acceleration structures must be created in the state D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, 
		// and must have resource flag D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS. The ALLOW_UNORDERED_ACCESS requirement simply acknowledges both: 
		//  - the system will be doing this type of access in its implementation of acceleration structure builds behind the scenes.
		//  - from the app point of view, synchronization of writes/reads to acceleration structures is accomplished using UAV barriers.
		{
			D3D12_RESOURCE_STATES initial_resoruce_state;
			if (GetRaytracingType(device) == RaytracingType::NATIVE)
			{
				initial_resoruce_state = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
			}
			else if (GetRaytracingType(device) == RaytracingType::FALLBACK)
			{
				initial_resoruce_state = device->m_fallback_native->GetAccelerationStructureResourceState();
			}

			internal::AllocateUAVBuffer(device, bottom_level_prebuild_info.ResultDataMaxSizeInBytes, &blas.m_native, initial_resoruce_state, L"BottomLevelAccelerationStructure");
			internal::AllocateUAVBuffer(device, top_level_prebuild_info.ResultDataMaxSizeInBytes, &tlas.m_native, initial_resoruce_state, L"TopLevelAccelerationStructure");
		}


		// Note on Emulated GPU pointers (AKA Wrapped pointers) requirement in Fallback Layer:
		// The primary point of divergence between the DXR API and the compute-based Fallback layer is the handling of GPU pointers. 
		// DXR fundamentally requires that GPUs be able to dynamically read from arbitrary addresses in GPU memory. 
		// The existing Direct Compute API today is more rigid than DXR and requires apps to explicitly inform the GPU what blocks of memory it will access with SRVs/UAVs.
		// In order to handle the requirements of DXR, the Fallback Layer uses the concept of Emulated GPU pointers, 
		// which requires apps to create views around all memory they will access for raytracing, 
		// but retains the DXR-like flexibility of only needing to bind the top level acceleration structure at DispatchRays.
		//
		// The Fallback Layer interface uses WRAPPED_GPU_POINTER to encapsulate the underlying pointer
		// which will either be an emulated GPU pointer for the compute - based path or a GPU_VIRTUAL_ADDRESS for the DXR path.

		// Create an instance desc for the bottom-level acceleration structure.
		if (GetRaytracingType(device) == RaytracingType::NATIVE)
		{
			D3D12_RAYTRACING_INSTANCE_DESC instance_desc = {};
			instance_desc.Transform[0][0] = instance_desc.Transform[1][1] = instance_desc.Transform[2][2] = 1;
			instance_desc.InstanceMask = 1;
			instance_desc.AccelerationStructure = blas.m_native->GetGPUVirtualAddress();

			internal::AllocateUploadBuffer(device, &instance_desc, sizeof(instance_desc), &tlas.m_instance_desc, L"InstanceDescs");
		}
		else if (GetRaytracingType(device) == RaytracingType::FALLBACK)
		{
			D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC instance_desc = {};
			instance_desc.Transform[0][0] = instance_desc.Transform[1][1] = instance_desc.Transform[2][2] = 1;
			instance_desc.InstanceMask = 1;
			UINT num_buffer_elements = static_cast<UINT>(bottom_level_prebuild_info.ResultDataMaxSizeInBytes) / sizeof(UINT32);
			instance_desc.AccelerationStructure = internal::CreateFallbackWrappedPointer(device, desc_heap, 0, blas.m_native, num_buffer_elements);

			internal::AllocateUploadBuffer(device, &instance_desc, sizeof(instance_desc), &tlas.m_instance_desc, L"InstanceDescs");
		}

		// TODO WHY HERE AND NOT EARLIER?
		// Create a wrapped pointer to the acceleration structure.
		if (GetRaytracingType(device) == RaytracingType::FALLBACK)
		{
			UINT num_buffer_elements = static_cast<UINT>(top_level_prebuild_info.ResultDataMaxSizeInBytes) / sizeof(UINT32);
			tlas.m_fallback_tlas_ptr = internal::CreateFallbackWrappedPointer(device, desc_heap, 1, tlas.m_native, num_buffer_elements);
		}

		// Bottom Level Acceleration Structure desc
		{
			bottom_level_build_desc .ScratchAccelerationStructureData = tlas.m_scratch->GetGPUVirtualAddress();
			bottom_level_build_desc.DestAccelerationStructureData = blas.m_native->GetGPUVirtualAddress();
		}

		// Top Level Acceleration Structure desc
		{
			top_level_build_desc.ScratchAccelerationStructureData = tlas.m_scratch->GetGPUVirtualAddress();
			top_level_build_desc.DestAccelerationStructureData = tlas.m_native->GetGPUVirtualAddress();
			top_level_build_desc.Inputs.InstanceDescs = tlas.m_instance_desc->GetGPUVirtualAddress();
		}

		auto BuildAccelerationStructure = [&](auto* raytracingCommandList)
		{
			raytracingCommandList->BuildRaytracingAccelerationStructure(&bottom_level_build_desc, 0, nullptr);
			cmd_list->m_native->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(blas.m_native));
			raytracingCommandList->BuildRaytracingAccelerationStructure(&top_level_build_desc, 0, nullptr);
		};

		// Build acceleration structure.
		if (GetRaytracingType(device) == RaytracingType::NATIVE)
		{
			BuildAccelerationStructure(cmd_list->m_native);
		}
		else if (GetRaytracingType(device) == RaytracingType::FALLBACK)
		{
			// Set the descriptor heaps to be used during acceleration structure build for the Fallback Layer.
			d3d12::BindDescriptorHeaps(cmd_list, { desc_heap }, 0, true);//TODO: note this non frame idx
			BuildAccelerationStructure(cmd_list->m_native_fallback);
		}

		return { blas, tlas };
	}

} /* wr::d3d12 */

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
					// desc_heap_idx = AllocateDescriptor(heap, increment_size, &bottomLevelDescriptor, index);
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
			// Mark the geometry as opaque. 
			// PERFORMANCE TIP: mark geometry as opaque whenever applicable as it can enable important ray processing optimizations.
			// Note: When rays encounter opaque geometry an any hit shader will not be executed whether it is present or not.
			geometry_descs[i].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
		}

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS build_flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

		// Get prebuild info bottom level
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottom_level_inputs;
		bottom_level_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		bottom_level_inputs.NumDescs = geometry.size();
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
		{
			D3D12_RESOURCE_STATES initial_resource_state;
			if (GetRaytracingType(device) == RaytracingType::NATIVE)
			{
				initial_resource_state = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
			}
			else if (GetRaytracingType(device) == RaytracingType::FALLBACK)
			{
				initial_resource_state = device->m_fallback_native->GetAccelerationStructureResourceState();
			}

			internal::AllocateUAVBuffer(device, blas.m_prebuild_info.ResultDataMaxSizeInBytes, &blas.m_native, initial_resource_state, L"BottomLevelAccelerationStructure");
		}

		// Bottom Level Acceleration Structure desc
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottom_level_build_desc = {};
		{
			bottom_level_build_desc.Inputs = bottom_level_inputs;
			bottom_level_build_desc.ScratchAccelerationStructureData = blas.m_scratch->GetGPUVirtualAddress();
			bottom_level_build_desc.DestAccelerationStructureData = blas.m_native->GetGPUVirtualAddress();
		}

		auto BuildAccelerationStructure = [&](auto* raytracingCommandList)
		{
			raytracingCommandList->BuildRaytracingAccelerationStructure(&bottom_level_build_desc, 0, nullptr);
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

		return blas;
	}

	AccelerationStructure CreateTopLevelAccelerationStructure(Device* device,
		CommandList* cmd_list,
		DescriptorHeap* desc_heap,
		std::vector<std::tuple<d3d12::AccelerationStructure, unsigned int, DirectX::XMMATRIX>> blas_list)
	{
		AccelerationStructure tlas = {};
		tlas.m_rebuild_scratch = true;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS build_flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS top_level_inputs;
		top_level_inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		top_level_inputs.Flags = build_flags;
		top_level_inputs.NumDescs = blas_list.size();
		top_level_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

		// Get prebuild info top level
		internal::UpdatePrebuildInfo(device, tlas, top_level_inputs);

		//LOG("{}", tlas.m_prebuild_info.ResultDataMaxSizeInBytes);

		// Allocate scratch resource
		internal::AllocateUAVBuffer(device,
			tlas.m_prebuild_info.ScratchDataSizeInBytes,
			&tlas.m_scratch,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			L"Acceleration Structure Scratch Resource");


		// Allocate acceleration structure buffer
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

			internal::AllocateUAVBuffer(device, tlas.m_prebuild_info.ResultDataMaxSizeInBytes, &tlas.m_native, initial_resoruce_state, L"TopLevelAccelerationStructure");
		}

		// Falback layer heap offset
		auto fallback_heap_idx = d3d12::settings::fallback_ptrs_offset;

		// Create the instances to the bottom level instances.
		if (GetRaytracingType(device) == RaytracingType::NATIVE)
		{
			std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instance_descs;
			for (auto it : blas_list)
			{
				auto blas = std::get<AccelerationStructure>(it);
				auto material = std::get<unsigned int>(it);
				auto transform = std::get<DirectX::XMMATRIX>(it);

				D3D12_RAYTRACING_INSTANCE_DESC instance_desc = {};

				XMStoreFloat3x4(reinterpret_cast<DirectX::XMFLOAT3X4*>(instance_desc.Transform), transform);

				instance_desc.InstanceMask = 1;
				instance_desc.InstanceID = material;
				instance_desc.AccelerationStructure = blas.m_native->GetGPUVirtualAddress();
				instance_descs.push_back(instance_desc);
			}

			internal::AllocateUploadBuffer(device, instance_descs.data(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * blas_list.size(), &tlas.m_instance_desc, L"InstanceDescs");
		}
		else if (GetRaytracingType(device) == RaytracingType::FALLBACK)
		{
			std::vector<D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC> instance_descs;
			for (auto it : blas_list)
			{
				auto blas = std::get<AccelerationStructure>(it);
				auto material = std::get<unsigned int>(it);
				auto transform = std::get<DirectX::XMMATRIX>(it);

				D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC instance_desc = {};

				XMStoreFloat3x4(reinterpret_cast<DirectX::XMFLOAT3X4*>(instance_desc.Transform), transform);

				instance_desc.InstanceMask = 1;
				instance_desc.InstanceID = material;
				UINT num_buffer_elements = static_cast<UINT>(blas.m_prebuild_info.ResultDataMaxSizeInBytes) / sizeof(UINT32);
				instance_desc.AccelerationStructure = internal::CreateFallbackWrappedPointer(device, desc_heap, fallback_heap_idx, blas.m_native, num_buffer_elements);

				instance_descs.push_back(instance_desc);

				fallback_heap_idx++;
			}

			internal::AllocateUploadBuffer(device, instance_descs.data(), sizeof(D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC) * instance_descs.size(), &tlas.m_instance_desc, L"InstanceDescs");
		}

		// TODO WHY HERE AND NOT EARLIER?
		// Create a wrapped pointer to the acceleration structure.
		if (GetRaytracingType(device) == RaytracingType::FALLBACK)
		{
			UINT num_buffer_elements = static_cast<UINT>(tlas.m_prebuild_info.ResultDataMaxSizeInBytes) / sizeof(UINT32);
			tlas.m_fallback_tlas_ptr = internal::CreateFallbackWrappedPointer(device, desc_heap, fallback_heap_idx, tlas.m_native, num_buffer_elements);
		}

		// Top Level Acceleration Structure desc
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC top_level_build_desc = {};
		{
			top_level_inputs.InstanceDescs = tlas.m_instance_desc->GetGPUVirtualAddress();
			top_level_build_desc.Inputs = top_level_inputs;
			top_level_build_desc.DestAccelerationStructureData = tlas.m_native->GetGPUVirtualAddress();
			top_level_build_desc.ScratchAccelerationStructureData = tlas.m_scratch->GetGPUVirtualAddress();
		}

		auto BuildAccelerationStructure = [&](auto* raytracingCommandList)
		{
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
			d3d12::BindDescriptorHeap(cmd_list, desc_heap, desc_heap->m_create_info.m_type, 0, true); //TODO: note this non frame idx
			BuildAccelerationStructure(cmd_list->m_native_fallback);
		}

		return tlas;
	}

	void DestroyAccelerationStructure(AccelerationStructure& structure)
	{
		SAFE_RELEASE(structure.m_scratch);
		SAFE_RELEASE(structure.m_native);
		SAFE_RELEASE(structure.m_instance_desc);
	}

	void UpdateTopLevelAccelerationStructure(AccelerationStructure& tlas, Device* device,
		CommandList* cmd_list,
		DescriptorHeap* desc_heap,
		std::vector<std::tuple<d3d12::AccelerationStructure, unsigned int, DirectX::XMMATRIX>> blas_list)
	{
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS build_flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS top_level_inputs;
		top_level_inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		top_level_inputs.Flags = build_flags;
		top_level_inputs.NumDescs = blas_list.size();
		top_level_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO old_prebuild_info = tlas.m_prebuild_info;

		internal::UpdatePrebuildInfo(device, tlas, top_level_inputs);
		if (tlas.m_rebuild_scratch)
		{
	//		LOG("{}", tlas.m_prebuild_info.ResultDataMaxSizeInBytes);

			if (tlas.m_scratch)
			{
				//tlas.m_scratch->Release();
			}

			// Allocate scratch resource

			tlas.m_rebuild_scratch = false;
		}

		bool rebuild_accel_structure = old_prebuild_info.ResultDataMaxSizeInBytes < tlas.m_prebuild_info.ResultDataMaxSizeInBytes ||
			old_prebuild_info.ScratchDataSizeInBytes < tlas.m_prebuild_info.ScratchDataSizeInBytes ||
			old_prebuild_info.UpdateScratchDataSizeInBytes < tlas.m_prebuild_info.UpdateScratchDataSizeInBytes;

		if (rebuild_accel_structure)
		{
			tlas = CreateTopLevelAccelerationStructure(device, cmd_list, desc_heap, blas_list);
		}
		else
		{
			// Get prebuild info top level

			// Falback layer heap offset
			auto fallback_heap_idx = d3d12::settings::fallback_ptrs_offset;

			// Create the instances to the bottom level instances.
			if (GetRaytracingType(device) == RaytracingType::NATIVE)
			{
				std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instance_descs;
				for (auto it : blas_list)
				{
					auto blas = std::get<AccelerationStructure>(it);
					auto material = std::get<unsigned int>(it);
					auto transform = std::get<DirectX::XMMATRIX>(it);

					D3D12_RAYTRACING_INSTANCE_DESC instance_desc = {};

					XMStoreFloat3x4(reinterpret_cast<DirectX::XMFLOAT3X4*>(instance_desc.Transform), transform);

					instance_desc.InstanceMask = 1;
					instance_desc.InstanceID = material;
					instance_desc.AccelerationStructure = blas.m_native->GetGPUVirtualAddress();
					instance_descs.push_back(instance_desc);
				}

				internal::UpdateUploadbuffer(instance_descs.data(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * blas_list.size(), tlas.m_instance_desc);
			}
			else if (GetRaytracingType(device) == RaytracingType::FALLBACK)
			{
				std::vector<D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC> instance_descs;
				for (auto it : blas_list)
				{
					auto blas = std::get<AccelerationStructure>(it);
					auto material = std::get<unsigned int>(it);
					auto transform = std::get<DirectX::XMMATRIX>(it);

					D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC instance_desc = {};

					XMStoreFloat3x4(reinterpret_cast<DirectX::XMFLOAT3X4*>(instance_desc.Transform), transform);

					instance_desc.InstanceMask = 1;
					instance_desc.InstanceID = material;
					UINT num_buffer_elements = static_cast<UINT>(blas.m_prebuild_info.ResultDataMaxSizeInBytes) / sizeof(UINT32);
					instance_desc.AccelerationStructure = internal::CreateFallbackWrappedPointer(device, desc_heap, fallback_heap_idx, blas.m_native, num_buffer_elements);

					instance_descs.push_back(instance_desc);

					fallback_heap_idx++;
				}

				internal::UpdateUploadbuffer(instance_descs.data(), sizeof(D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC) * instance_descs.size(), tlas.m_instance_desc);
			}

			// Create a wrapped pointer to the acceleration structure.
			if (GetRaytracingType(device) == RaytracingType::FALLBACK)
			{
				UINT num_buffer_elements = static_cast<UINT>(tlas.m_prebuild_info.ResultDataMaxSizeInBytes) / sizeof(UINT32);
				tlas.m_fallback_tlas_ptr = internal::CreateFallbackWrappedPointer(device, desc_heap, fallback_heap_idx, tlas.m_native, num_buffer_elements);
			}

			// Top Level Acceleration Structure desc
			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC top_level_build_desc = {};
			{
				cmd_list->m_native->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(tlas.m_native));

				top_level_inputs.InstanceDescs = tlas.m_instance_desc->GetGPUVirtualAddress();
				top_level_build_desc.Inputs = top_level_inputs;
				top_level_build_desc.SourceAccelerationStructureData = tlas.m_native->GetGPUVirtualAddress();
				top_level_build_desc.DestAccelerationStructureData = tlas.m_native->GetGPUVirtualAddress();
				top_level_build_desc.ScratchAccelerationStructureData = tlas.m_scratch->GetGPUVirtualAddress();
			}

			auto BuildAccelerationStructure = [&](auto* raytracingCommandList)
			{
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
				d3d12::BindDescriptorHeap(cmd_list, desc_heap, desc_heap->m_create_info.m_type, 0, true); //TODO: note this non frame idx
				BuildAccelerationStructure(cmd_list->m_native_fallback);
			}
		}
	}

	void SetName(std::pair<AccelerationStructure, AccelerationStructure> acceleration_structure, std::wstring name)
	{
		acceleration_structure.first.m_native->SetName((name + L" - Bottom Level Acceleration Structure").c_str());
		acceleration_structure.second.m_native->SetName((name + L" - Top Level Acceleration Structure").c_str());
	}

} /* wr::d3d12 */
#include "d3d12_functions.hpp"

#include "d3d12_defines.hpp"

namespace d3d12
{

	Heap<HeapOptimization::SMALL_BUFFERS>* CreateHeap_SBO(Device* device, std::uint64_t size_in_bytes, ResourceType resource_type, unsigned int versioning_count)
	{
		auto heap = new Heap<HeapOptimization::SMALL_BUFFERS>();
		heap->m_mapped = false;
		heap->m_versioning_count = versioning_count;
		heap->m_current_offset = 0;

		auto aligned_size = SizeAlign(size_in_bytes, 65536);

		auto heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto resource_desc = CD3DX12_RESOURCE_DESC::Buffer(aligned_size, D3D12_RESOURCE_FLAG_NONE);

		TRY_M(device->m_native->CreateCommittedResource(
			&heap_properties,
			D3D12_HEAP_FLAG_NONE,
			&resource_desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&heap->m_native)),
			"Failed to create small buffer optimized heap.");

		return heap;
	}

	Heap<HeapOptimization::BIG_BUFFERS>* CreateHeap_BBO(Device* device, std::uint64_t size_in_bytes, ResourceType resource_type, unsigned int versioning_count)
	{
		auto heap = new Heap<HeapOptimization::BIG_BUFFERS>();
		heap->m_mapped = false;
		heap->m_versioning_count = versioning_count;
		heap->m_current_offset = 0;

		D3D12_HEAP_PROPERTIES heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		CD3DX12_HEAP_DESC desc = {};
		desc.SizeInBytes = size_in_bytes;
		desc.Properties = heap_properties;
		desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		switch (resource_type)
		{
		case ResourceType::BUFFER:
			desc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
			break;
		case ResourceType::TEXTURE:
			desc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
			break;
		case ResourceType::RT_DS:
			desc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
			break;
		}

		TRY_M(device->m_native->CreateHeap(&desc, IID_PPV_ARGS(&heap->m_native)),
			"Failed to create heap.");

		return heap;
	}

	HeapResource* AllocConstantBuffer(Heap<HeapOptimization::SMALL_BUFFERS>* heap, std::uint64_t size_in_bytes)
	{
		auto cb = new HeapResource();

		auto aligned_size = SizeAlign(size_in_bytes, 256);
		cb->m_unaligned_size = size_in_bytes;
		cb->m_gpu_addresses.resize(heap->m_versioning_count);
		cb->m_begin_offset = heap->m_current_offset;

		for (auto i = 0; i < heap->m_versioning_count; i++)
		{
			cb->m_gpu_addresses[i] = heap->m_native->GetGPUVirtualAddress() + heap->m_current_offset;
			heap->m_current_offset += aligned_size;
		}

		// If the heap is supposed to be mapped immediatly map the new resource.
		if (heap->m_mapped)
		{
			cb->m_cpu_addresses = std::vector<std::uint8_t*>(heap->m_versioning_count);
			auto&& addresses = cb->m_cpu_addresses.value();
			for (auto i = 0; i < heap->m_versioning_count; i++)
			{
				addresses[i] = heap->m_cpu_address + (cb->m_begin_offset + (SizeAlign(cb->m_unaligned_size, 255) * i));
			}
		}
		
		cb->m_heap_vector_location = heap->m_resources.size();
		heap->m_resources.push_back(cb);

		return cb;
	}

	HeapResource* AllocConstantBuffer(Heap<HeapOptimization::BIG_BUFFERS>* heap, std::uint64_t size_in_bytes)
	{
		auto cb = new HeapResource();
		decltype(Device::m_native) n_device;
		heap->m_native->GetDevice(IID_PPV_ARGS(&n_device));
		cb->m_unaligned_size = size_in_bytes;

		auto aligned_size_in_bytes = SizeAlign(size_in_bytes, 65536);

		CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(aligned_size_in_bytes, D3D12_RESOURCE_FLAG_NONE);
		cb->m_gpu_addresses.resize(heap->m_versioning_count);
		cb->m_heap_vector_location = heap->m_resources.size();

		std::vector<ID3D12Resource*> temp_resources(heap->m_versioning_count);
		for (auto i = 0; i < heap->m_versioning_count; i++)
		{
			TRY_M(n_device->CreatePlacedResource(heap->m_native, heap->m_current_offset, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&temp_resources[i])),
				"Failed to create constant buffer placed resource.");

			heap->m_current_offset += aligned_size_in_bytes;

			cb->m_gpu_addresses[i] = temp_resources[i]->GetGPUVirtualAddress();
		}

		// If the heap is supposed to be mapped immediatly map the new resource.
		if (heap->m_mapped)
		{
			cb->m_cpu_addresses = std::vector<std::uint8_t*>(heap->m_versioning_count);
			auto&& addresses = cb->m_cpu_addresses.value();

			CD3DX12_RANGE read_range(0, 0);
			for (auto i = 0; i < heap->m_versioning_count; i++)
			{
				TRY_M(temp_resources[i]->Map(0, &read_range, reinterpret_cast<void**>(&addresses[i])),
					"Failed to map resource.");
			}
		}

		heap->m_resources.push_back(std::make_pair(cb, temp_resources));

		return cb;
	}

	void MapHeap(Heap<HeapOptimization::SMALL_BUFFERS>* heap)
	{
		heap->m_mapped = true;

		CD3DX12_RANGE read_range(0, 0);
		std::uint8_t* address;
		TRY_M(heap->m_native->Map(0, &read_range, reinterpret_cast<void**>(&address)),
			"Failed to map resource.");

		heap->m_cpu_address = address;

		for (auto& handle : heap->m_resources)
		{
			handle->m_cpu_addresses = std::vector<std::uint8_t*>(heap->m_versioning_count);
			auto&& addresses = handle->m_cpu_addresses.value();

			for (auto i = 0; i < heap->m_versioning_count; i++)
			{
				addresses[i] = address + (handle->m_begin_offset + (SizeAlign(handle->m_unaligned_size, 255) * i));
			}
		}
	}

	void MapHeap(Heap<HeapOptimization::BIG_BUFFERS>* heap)
	{
		heap->m_mapped = true;

		for (auto resource : heap->m_resources)
		{
			resource.first->m_cpu_addresses = std::vector<std::uint8_t*>(heap->m_versioning_count);
			auto&& addresses = resource.first->m_cpu_addresses.value();

			CD3DX12_RANGE read_range(0, 0);
			for (auto i = 0; i < heap->m_versioning_count; i++)
			{
				TRY_M(resource.second[i]->Map(0, &read_range, reinterpret_cast<void**>(&addresses[i])),
					"Failed to map resource.");
			}
		}
	}

	void UnmapHeap(Heap<HeapOptimization::SMALL_BUFFERS>* heap)
	{
		heap->m_mapped = false;

		CD3DX12_RANGE read_range(0, 0);
		heap->m_native->Unmap(0, &read_range);

		// For safety set the optional cpu addresses back to nullopt.
		for (auto& resource : heap->m_resources)
		{
			resource->m_cpu_addresses = std::nullopt;
		}
	}

	void UnmapHeap(Heap<HeapOptimization::BIG_BUFFERS>* heap)
	{
		heap->m_mapped = false;

		CD3DX12_RANGE read_range(0, 0);

		for (auto& resource : heap->m_resources)
		{
			resource.first->m_cpu_addresses = std::nullopt;
			for (auto& n_resource : resource.second)
			{
				n_resource->Unmap(0, &read_range);
			}
		}
	}

	void MakeResident(Heap<HeapOptimization::SMALL_BUFFERS>* heap)
	{
		decltype(Device::m_native) n_device;
		heap->m_native->GetDevice(IID_PPV_ARGS(&n_device));

		std::array<ID3D12Pageable*, 1> objects { heap->m_native };
		n_device->MakeResident(1, objects.data());
	}

	void MakeResident(Heap<HeapOptimization::BIG_BUFFERS>* heap)
	{
		decltype(Device::m_native) n_device;
		heap->m_native->GetDevice(IID_PPV_ARGS(&n_device));

		std::array<ID3D12Pageable*, 1> objects{ heap->m_native };
		n_device->MakeResident(1, objects.data());
	}

	void EnqueueMakeResident(Heap<HeapOptimization::SMALL_BUFFERS>* heap, Fence* fence)
	{
		decltype(Device::m_native) n_device;
		heap->m_native->GetDevice(IID_PPV_ARGS(&n_device));

		std::array<ID3D12Pageable*, 1> objects{ heap->m_native };
		n_device->EnqueueMakeResident(D3D12_RESIDENCY_FLAG_DENY_OVERBUDGET, 1, objects.data(), fence->m_native, fence->m_fence_value);
	}

	void EnqueueMakeResident(Heap<HeapOptimization::BIG_BUFFERS>* heap, Fence* fence)
	{
		decltype(Device::m_native) n_device;
		heap->m_native->GetDevice(IID_PPV_ARGS(&n_device));

		std::array<ID3D12Pageable*, 1> objects{ heap->m_native };
		n_device->EnqueueMakeResident(D3D12_RESIDENCY_FLAG_DENY_OVERBUDGET, 1, objects.data(), fence->m_native, fence->m_fence_value);
	}

	void Evict(Heap<HeapOptimization::SMALL_BUFFERS>* heap)
	{
		decltype(Device::m_native) n_device;
		heap->m_native->GetDevice(IID_PPV_ARGS(&n_device));

		std::array<ID3D12Pageable*, 1> objects{ heap->m_native };
		n_device->Evict(1, objects.data());
	}

	void Evict(Heap<HeapOptimization::BIG_BUFFERS>* heap)
	{
		decltype(Device::m_native) n_device;
		heap->m_native->GetDevice(IID_PPV_ARGS(&n_device));

		std::array<ID3D12Pageable*, 1> objects{ heap->m_native };
		n_device->Evict(1, objects.data());
	}

	void Destroy(Heap<HeapOptimization::SMALL_BUFFERS>* heap)
	{
		UnmapHeap(heap);

		SAFE_RELEASE(heap->m_native);
		for (auto& resource : heap->m_resources)
		{
			delete resource;
		}
		delete heap;
	}

	void Destroy(Heap<HeapOptimization::BIG_BUFFERS>* heap)
	{
		UnmapHeap(heap);

		SAFE_RELEASE(heap->m_native);
		for (auto& resource : heap->m_resources)
		{
			delete resource.first;
			for (auto& n_resource : resource.second)
			{
				delete n_resource;
			}
		}
		delete heap;
	}

	void UpdateConstantBuffer(HeapResource* buffer, unsigned int frame_idx, void* data, std::uint64_t size_in_bytes)
	{
		if (buffer->m_cpu_addresses.has_value())
		{
			auto&& addresses = buffer->m_cpu_addresses.value();
			std::memcpy(addresses[frame_idx], data, buffer->m_unaligned_size);
		}
		else
		{
			LOGW("Tried updating a unmapped constant buffer resource!");
		}
	}

} /* d3d12 */
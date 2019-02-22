#include "d3d12_functions.hpp"

#include "d3d12_defines.hpp"
#include "../util/bitmap_allocator.hpp"

namespace wr::d3d12
{

	namespace internal
	{
		static const auto MakeResidentSingle = [](auto heap)
		{
			decltype(Device::m_native) n_device;
			heap->m_native->GetDevice(IID_PPV_ARGS(&n_device));

			std::array<ID3D12Pageable*, 1> objects{ heap->m_native };
			n_device->MakeResident(1, objects.data());
		};

		static const auto EnqueueMakeResidentSingle = [](auto heap, auto fence)
		{
			decltype(Device::m_native) n_device;
			heap->m_native->GetDevice(IID_PPV_ARGS(&n_device));

			std::array<ID3D12Pageable*, 1> objects{ heap->m_native };
			n_device->EnqueueMakeResident(D3D12_RESIDENCY_FLAG_DENY_OVERBUDGET, 1, objects.data(), fence->m_native, fence->m_fence_value);
		};

		static const auto EvictSingle = [](auto heap)
		{
			decltype(Device::m_native) n_device;
			heap->m_native->GetDevice(IID_PPV_ARGS(&n_device));

			std::array<ID3D12Pageable*, 1> objects{ heap->m_native };
			n_device->Evict(1, objects.data());
		};
	}

	Heap<HeapOptimization::SMALL_BUFFERS>* CreateHeap_SBO(Device* device, std::uint64_t size_in_bytes, ResourceType resource_type, unsigned int versioning_count)
	{
		auto heap = new Heap<HeapOptimization::SMALL_BUFFERS>();
		heap->m_mapped = false;
		heap->m_versioning_count = versioning_count;
		heap->m_current_offset = 0;
		heap->m_alignment = 256;

		auto aligned_size = SizeAlign(size_in_bytes, 65536);

		heap->m_heap_size = aligned_size;

		auto page_frame_count = SizeAlign(heap->m_heap_size / heap->m_alignment, 64) / 64;

		heap->m_bitmap.resize(page_frame_count);

		for (int i = 0; i < heap->m_bitmap.size(); ++i) 
		{
			heap->m_bitmap[i] = 0xffffffffffffffff;
		}

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
		heap->m_alignment = 65536;

		auto aligned_size = SizeAlign(size_in_bytes, 65536);

		heap->m_heap_size = aligned_size;

		auto page_frame_count = SizeAlign(heap->m_heap_size / heap->m_alignment, 64) / 64;

		heap->m_bitmap.resize(page_frame_count);

		for (int i = 0; i < heap->m_bitmap.size(); ++i) 
		{
			heap->m_bitmap[i] = 0xffffffffffffffff;
		}

		D3D12_HEAP_PROPERTIES heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		CD3DX12_HEAP_DESC desc = {};
		desc.SizeInBytes = heap->m_heap_size;
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

	Heap<HeapOptimization::SMALL_STATIC_BUFFERS>* CreateHeap_SSBO(Device * device, std::uint64_t size_in_bytes, ResourceType resource_type, unsigned int versioning_count)
	{
		auto heap = new Heap<HeapOptimization::SMALL_STATIC_BUFFERS>();
		heap->m_mapped = false;
		heap->m_versioning_count = versioning_count;
		heap->m_current_offset = 0;
		heap->m_alignment = 256;

		auto aligned_size = SizeAlign(size_in_bytes, 65536);

		heap->m_heap_size = aligned_size;

		auto page_frame_count = SizeAlign(heap->m_heap_size / heap->m_alignment, 64) / 64;

		heap->m_bitmap.resize(page_frame_count);

		for (int i = 0; i < heap->m_bitmap.size(); ++i) 
		{
			heap->m_bitmap[i] = 0xffffffffffffffff;
		}

		auto heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		auto resource_desc = CD3DX12_RESOURCE_DESC::Buffer(aligned_size, D3D12_RESOURCE_FLAG_NONE);

		TRY_M(device->m_native->CreateCommittedResource(
			&heap_properties,
			D3D12_HEAP_FLAG_NONE,
			&resource_desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&heap->m_native)),
			"Failed to create small buffer optimized heap.");

		heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		resource_desc = CD3DX12_RESOURCE_DESC::Buffer(aligned_size, D3D12_RESOURCE_FLAG_NONE);

		TRY_M(device->m_native->CreateCommittedResource(
			&heap_properties,
			D3D12_HEAP_FLAG_NONE,
			&resource_desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&heap->m_staging_buffer)),
			"Failed to create small buffer optimized heap.");

		heap->m_staging_buffer->Map(0, &CD3DX12_RANGE(0, 0), reinterpret_cast<void**>(&heap->m_cpu_address));

		return heap;
	}

	Heap<HeapOptimization::BIG_STATIC_BUFFERS>* CreateHeap_BSBO(Device * device, std::uint64_t size_in_bytes, ResourceType resource_type, unsigned int versioning_count)
	{
		auto heap = new Heap<HeapOptimization::BIG_STATIC_BUFFERS>();
		heap->m_mapped = false;
		heap->m_versioning_count = versioning_count;
		heap->m_current_offset = 0;
		heap->m_alignment = 65536;

		auto aligned_size = SizeAlign(size_in_bytes, 65536);

		heap->m_heap_size = aligned_size;

		auto page_frame_count = SizeAlign(heap->m_heap_size / heap->m_alignment, 64) / 64;

		heap->m_bitmap.resize(page_frame_count);

		for (int i = 0; i < heap->m_bitmap.size(); ++i) 
		{
			heap->m_bitmap[i] = 0xffffffffffffffff;
		}

		D3D12_HEAP_PROPERTIES heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

		CD3DX12_HEAP_DESC desc = {};
		desc.SizeInBytes = heap->m_heap_size;
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

		heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto resource_desc = CD3DX12_RESOURCE_DESC::Buffer(aligned_size, D3D12_RESOURCE_FLAG_NONE);

		TRY_M(device->m_native->CreateCommittedResource(
			&heap_properties,
			D3D12_HEAP_FLAG_NONE,
			&resource_desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&heap->m_staging_buffer)),
			"Failed to create heap staging buffer.");

		heap->m_staging_buffer->Map(0, &CD3DX12_RANGE(0, 0), reinterpret_cast<void**>(&heap->m_cpu_address));

		return heap;
	}

	HeapResource* AllocConstantBuffer(Heap<HeapOptimization::SMALL_BUFFERS>* heap, std::uint64_t size_in_bytes)
	{
		auto cb = new HeapResource();

		auto aligned_size = SizeAlign(size_in_bytes, 256);
		cb->m_unaligned_size = size_in_bytes;
		cb->m_gpu_addresses.resize(heap->m_versioning_count);

		auto frame_count = heap->m_heap_size / heap->m_alignment;
		auto needed_frames = aligned_size / heap->m_alignment*heap->m_versioning_count;

		auto start_frame = util::FindFreePage(heap->m_bitmap, frame_count, needed_frames);

		if (!start_frame.has_value())
		{
			delete cb;
			return nullptr;
		}

		for (std::uint64_t i = 0; i < needed_frames; ++i) 
		{
			util::ClearPage(heap->m_bitmap, start_frame.value() + i);
		}

		heap->m_current_offset = start_frame.value() * heap->m_alignment;

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

		cb->m_heap_sbo = heap;
		cb->m_resource_heap_optimization = HeapOptimization::SMALL_BUFFERS;

		return cb;
	}

	HeapResource* AllocConstantBuffer(Heap<HeapOptimization::BIG_BUFFERS>* heap, std::uint64_t size_in_bytes)
	{
		auto cb = new HeapResource();
		decltype(Device::m_native) n_device;
		heap->m_native->GetDevice(IID_PPV_ARGS(&n_device));
		cb->m_unaligned_size = size_in_bytes;

		auto aligned_size_in_bytes = SizeAlign(size_in_bytes, 65536);

		auto frame_count = heap->m_heap_size / heap->m_alignment;
		auto needed_frames = aligned_size_in_bytes / heap->m_alignment*heap->m_versioning_count;

		auto start_frame = util::FindFreePage(heap->m_bitmap, frame_count, needed_frames);

		if (!start_frame.has_value())
		{
			delete cb;
			return nullptr;
		}

		for (std::uint64_t i = 0; i < needed_frames; ++i) 
		{
			util::ClearPage(heap->m_bitmap, start_frame.value() + i);
		}

		heap->m_current_offset = start_frame.value() * heap->m_alignment;

		CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(aligned_size_in_bytes, D3D12_RESOURCE_FLAG_NONE);
		cb->m_gpu_addresses.resize(heap->m_versioning_count);
		cb->m_heap_vector_location = heap->m_resources.size();

		cb->m_begin_offset = heap->m_current_offset;

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

		cb->m_heap_bbo = heap;
		cb->m_resource_heap_optimization = HeapOptimization::BIG_BUFFERS;

		return cb;
	}

	HeapResource* AllocByteAddressBuffer(Heap<HeapOptimization::BIG_BUFFERS>* heap, std::uint64_t size_in_bytes)
	{
		auto cb = new HeapResource();
		decltype(Device::m_native) n_device;
		heap->m_native->GetDevice(IID_PPV_ARGS(&n_device));
		cb->m_unaligned_size = size_in_bytes;

		auto aligned_size_in_bytes = SizeAlign(size_in_bytes, 65536);

		auto frame_count = heap->m_heap_size / heap->m_alignment;
		auto needed_frames = aligned_size_in_bytes / heap->m_alignment*heap->m_versioning_count;

		auto start_frame = util::FindFreePage(heap->m_bitmap, frame_count, needed_frames);

		if (!start_frame.has_value())
		{
			delete cb;
			return nullptr;
		}

		for (std::uint64_t i = 0; i < needed_frames; ++i)
		{
			util::ClearPage(heap->m_bitmap, start_frame.value() + i);
		}

		heap->m_current_offset = start_frame.value() * heap->m_alignment;

		CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(aligned_size_in_bytes, D3D12_RESOURCE_FLAG_NONE);
		cb->m_gpu_addresses.resize(heap->m_versioning_count);
		cb->m_heap_vector_location = heap->m_resources.size();

		cb->m_begin_offset = heap->m_current_offset;

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

		cb->m_heap_bbo = heap;
		cb->m_resource_heap_optimization = HeapOptimization::BIG_BUFFERS;

		return cb;
	}

	HeapResource* AllocStructuredBuffer(Heap<HeapOptimization::BIG_STATIC_BUFFERS>* heap, std::uint64_t size_in_bytes, std::uint64_t stride, bool used_as_uav)
	{
		auto cb = new HeapResource();
		decltype(Device::m_native) n_device;
		heap->m_native->GetDevice(IID_PPV_ARGS(&n_device));
		cb->m_unaligned_size = size_in_bytes;
		cb->m_stride = stride;
		cb->m_used_as_uav = used_as_uav;

		auto aligned_size_in_bytes = SizeAlign(size_in_bytes, 65536);

		auto frame_count = heap->m_heap_size / heap->m_alignment;
		auto needed_frames = aligned_size_in_bytes / heap->m_alignment*heap->m_versioning_count;

		auto start_frame = util::FindFreePage(heap->m_bitmap, frame_count, needed_frames);

		if (!start_frame.has_value())
		{
			delete cb;
			return nullptr;
		}

		for (std::uint64_t i = 0; i < needed_frames; ++i) 
		{
			util::ClearPage(heap->m_bitmap, start_frame.value() + i);
		}

		heap->m_current_offset = start_frame.value() * heap->m_alignment;
				
		CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(
			aligned_size_in_bytes, 
			used_as_uav ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE);

		cb->m_gpu_addresses.resize(heap->m_versioning_count);
		cb->m_heap_vector_location = heap->m_resources.size();

		cb->m_begin_offset = heap->m_current_offset;

		std::vector<ID3D12Resource*> temp_resources(heap->m_versioning_count);
		for (auto i = 0; i < heap->m_versioning_count; i++)
		{
			TRY_M(n_device->CreatePlacedResource(
				heap->m_native, 
				heap->m_current_offset, 
				&desc, 
				used_as_uav ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS : D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 
				nullptr, 
				IID_PPV_ARGS(&temp_resources[i])),
				"Failed to create constant buffer placed resource.");			

			heap->m_current_offset += aligned_size_in_bytes;

			cb->m_gpu_addresses[i] = temp_resources[i]->GetGPUVirtualAddress();
			cb->m_states.push_back(used_as_uav ? ResourceState::UNORDERED_ACCESS : ResourceState::PIXEL_SHADER_RESOURCE);
		}
		

		heap->m_resources.push_back(std::make_pair(cb, temp_resources));

		cb->m_heap_bsbo = heap;
		cb->m_resource_heap_optimization = HeapOptimization::BIG_STATIC_BUFFERS;

		return cb;
	}

	HeapResource * AllocGenericBuffer(Heap<HeapOptimization::BIG_STATIC_BUFFERS>* heap, std::uint64_t size_in_bytes)
	{
		auto cb = new HeapResource();
		decltype(Device::m_native) n_device;
		heap->m_native->GetDevice(IID_PPV_ARGS(&n_device));
		cb->m_unaligned_size = size_in_bytes;

		auto aligned_size_in_bytes = SizeAlign(size_in_bytes, 65536);

		auto frame_count = heap->m_heap_size / heap->m_alignment;
		auto needed_frames = aligned_size_in_bytes / heap->m_alignment*heap->m_versioning_count;

		auto start_frame = util::FindFreePage(heap->m_bitmap, frame_count, needed_frames);

		if (!start_frame.has_value())
		{
			delete cb;
			return nullptr;
		}

		for (std::uint64_t i = 0; i < needed_frames; ++i) 
		{
			util::ClearPage(heap->m_bitmap, start_frame.value() + i);
		}

		heap->m_current_offset = start_frame.value() * heap->m_alignment;

		CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(aligned_size_in_bytes, D3D12_RESOURCE_FLAG_NONE);
		cb->m_gpu_addresses.resize(heap->m_versioning_count);
		cb->m_heap_vector_location = heap->m_resources.size();

		cb->m_begin_offset = heap->m_current_offset;

		std::vector<ID3D12Resource*> temp_resources(heap->m_versioning_count);
		for (auto i = 0; i < heap->m_versioning_count; i++)
		{
			TRY_M(n_device->CreatePlacedResource(heap->m_native, heap->m_current_offset, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&temp_resources[i])),
				"Failed to create constant buffer placed resource.");

			heap->m_current_offset += aligned_size_in_bytes;

			cb->m_gpu_addresses[i] = temp_resources[i]->GetGPUVirtualAddress();
		}

		heap->m_resources.push_back(std::make_pair(cb, temp_resources));

		cb->m_heap_bsbo = heap;
		cb->m_resource_heap_optimization = HeapOptimization::BIG_STATIC_BUFFERS;

		return cb;
	}

	void SetName(Heap<HeapOptimization::SMALL_BUFFERS>* heap, std::wstring name)
	{
		heap->m_native->SetName(name.c_str());
	}

	void SetName(Heap<HeapOptimization::BIG_BUFFERS>* heap, std::wstring name)
	{
		heap->m_native->SetName(name.c_str());
	}

	void SetName(Heap<HeapOptimization::SMALL_STATIC_BUFFERS>* heap, std::wstring name)
	{
		heap->m_native->SetName(name.c_str());
	}

	void SetName(Heap<HeapOptimization::BIG_STATIC_BUFFERS>* heap, std::wstring name)
	{
		heap->m_native->SetName(name.c_str());
	}


	void DeallocConstantBuffer(Heap<HeapOptimization::SMALL_BUFFERS>* heap, HeapResource * heapResource)
	{
		

		std::vector<HeapResource*>::iterator it;

		for (it = heap->m_resources.begin(); it != heap->m_resources.end(); ++it)
		{
			if ((*it) == heapResource)
				break;
		}

		if (it == heap->m_resources.end()) 
		{
			return;
		}
		
		heap->m_resources.erase(it);
		

		for (int i = 0; i < heap->m_resources.size(); ++i)
		{
			heap->m_resources[i]->m_heap_vector_location = i;
		}

		std::uint64_t frame = heapResource->m_begin_offset / heap->m_alignment;
		std::uint64_t frame_count = SizeAlign(heapResource->m_unaligned_size, 256) / heap->m_alignment * heap->m_versioning_count;

		for (int i = 0; i < frame_count; ++i) 
		{
			util::SetPage(heap->m_bitmap, frame + i);
		}

		delete heapResource;
	}

	void DeallocConstantBuffer(Heap<HeapOptimization::BIG_BUFFERS>* heap, HeapResource * heapResource)
	{
		std::vector<std::pair<HeapResource*, std::vector<ID3D12Resource*>>>::iterator it;

		for (it = heap->m_resources.begin(); it != heap->m_resources.end(); ++it) 
		{
			if ((*it).first == heapResource)
				break;
		}

		if (it == heap->m_resources.end()) 
		{
			return;
		}

		
		for (int i = 0; i < (*it).second.size(); ++i) 
		{
			delete (*it).second[i];
		}
		heap->m_resources.erase(it);
		

		for (int i = 0; i < heap->m_resources.size(); ++i) 
		{
			heap->m_resources[i].first->m_heap_vector_location = i;
		}

		std::uint64_t frame = heapResource->m_begin_offset / heap->m_alignment;
		std::uint64_t frame_count = SizeAlign(heapResource->m_unaligned_size, heap->m_alignment) / heap->m_alignment * heap->m_versioning_count;

		for (int i = 0; i < frame_count; ++i) 
		{
			util::SetPage(heap->m_bitmap, frame + i);
		}

		delete heapResource;

	}

	void DeallocBuffer(Heap<HeapOptimization::BIG_STATIC_BUFFERS>* heap, HeapResource * heapResource)
	{
		std::vector<std::pair<HeapResource*, std::vector<ID3D12Resource*>>>::iterator it;

		for (it = heap->m_resources.begin(); it != heap->m_resources.end(); ++it) 
		{
			if ((*it).first == heapResource)
				break;
		}

		if (it == heap->m_resources.end())
		{
			return;
		}

		for (int i = 0; i < (*it).second.size(); ++i) 
		{
			delete (*it).second[i];
		}
		heap->m_resources.erase(it);


		for (int i = 0; i < heap->m_resources.size(); ++i) 
		{
			heap->m_resources[i].first->m_heap_vector_location = i;
		}

		std::uint64_t frame = heapResource->m_begin_offset / heap->m_alignment;
		std::uint64_t frame_count = SizeAlign(heapResource->m_unaligned_size, heap->m_alignment) / heap->m_alignment * heap->m_versioning_count;

		for (int i = 0; i < frame_count; ++i) 
		{
			util::SetPage(heap->m_bitmap, frame + i);
		}

		delete heapResource;
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
		internal::MakeResidentSingle(heap);
	}

	void MakeResident(Heap<HeapOptimization::BIG_BUFFERS>* heap)
	{
		internal::MakeResidentSingle(heap);
	}

	void MakeResident(Heap<HeapOptimization::SMALL_STATIC_BUFFERS>* heap)
	{
		internal::MakeResidentSingle(heap);
	}

	void MakeResident(Heap<HeapOptimization::BIG_STATIC_BUFFERS>* heap)
	{
		internal::MakeResidentSingle(heap);
	}

	void EnqueueMakeResident(Heap<HeapOptimization::SMALL_BUFFERS>* heap, Fence* fence)
	{
		internal::EnqueueMakeResidentSingle(heap, fence);
	}

	void EnqueueMakeResident(Heap<HeapOptimization::BIG_BUFFERS>* heap, Fence* fence)
	{
		internal::EnqueueMakeResidentSingle(heap, fence);
	}

	void EnqueueMakeResident(Heap<HeapOptimization::SMALL_STATIC_BUFFERS>* heap, Fence* fence)
	{
		internal::EnqueueMakeResidentSingle(heap, fence);
	}

	void EnqueueMakeResident(Heap<HeapOptimization::BIG_STATIC_BUFFERS>* heap, Fence* fence)
	{
		internal::EnqueueMakeResidentSingle(heap, fence);
	}

	void Evict(Heap<HeapOptimization::SMALL_BUFFERS>* heap)
	{
		internal::EvictSingle(heap);
	}

	void Evict(Heap<HeapOptimization::BIG_BUFFERS>* heap)
	{
		internal::EvictSingle(heap);
	}

	void Evict(Heap<HeapOptimization::SMALL_STATIC_BUFFERS>* heap)
	{
		internal::EvictSingle(heap);
	}

	void Evict(Heap<HeapOptimization::BIG_STATIC_BUFFERS>* heap)
	{
		internal::EvictSingle(heap);
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

	void Destroy(Heap<HeapOptimization::SMALL_STATIC_BUFFERS>* heap)
	{
		//UnmapHeap(heap);

		SAFE_RELEASE(heap->m_native);
		SAFE_RELEASE(heap->m_staging_buffer);
		for (auto& resource : heap->m_resources)
		{
			delete resource;
		}
		delete heap;
	}

	void Destroy(Heap<HeapOptimization::BIG_STATIC_BUFFERS>* heap)
	{
		//UnmapHeap(heap);

		SAFE_RELEASE(heap->m_native);
		SAFE_RELEASE(heap->m_staging_buffer);
		for (auto& resource : heap->m_resources)
		{
			delete resource.first;
			for (auto& n_resource : resource.second)
			{
				SAFE_RELEASE(n_resource);
			}
		}
		delete heap;
	}

	void UpdateConstantBuffer(HeapResource* buffer, unsigned int frame_idx, void* data, std::uint64_t size_in_bytes)
	{
		if (buffer->m_cpu_addresses.has_value())
		{
			auto&& addresses = buffer->m_cpu_addresses.value();
			std::memcpy(addresses[frame_idx], data, size_in_bytes);
		}
		else
		{
			LOGW("Tried updating a unmapped constant buffer resource!");
		}
	}

	void UpdateStructuredBuffer(HeapResource * buffer, unsigned int frame_idx, void * data, std::uint64_t size_in_bytes, std::uint64_t offset, std::uint64_t stride, CommandList * cmd_list)
	{
		if (buffer->m_resource_heap_optimization != HeapOptimization::BIG_STATIC_BUFFERS)
		{
			return;
		}

		std::size_t aligned_size = SizeAlign(buffer->m_unaligned_size, 65536);

		memcpy(buffer->m_heap_bsbo->m_cpu_address + buffer->m_begin_offset + offset + aligned_size * frame_idx, data, size_in_bytes);

		buffer->m_stride = stride;

		if (size_in_bytes != 0) {
			ID3D12Resource* resource = buffer->m_heap_bsbo->m_resources[buffer->m_heap_vector_location].second[frame_idx];

			cmd_list->m_native->ResourceBarrier(1,
				&CD3DX12_RESOURCE_BARRIER::Transition(resource,
					static_cast<D3D12_RESOURCE_STATES>(buffer->m_states[frame_idx]),
					D3D12_RESOURCE_STATE_COPY_DEST));
			
			cmd_list->m_native->CopyBufferRegion(resource, 
				offset, 
				buffer->m_heap_bsbo->m_staging_buffer, 
				buffer->m_begin_offset + offset + aligned_size * frame_idx, 
				size_in_bytes);

			
			cmd_list->m_native->ResourceBarrier(1,
				&CD3DX12_RESOURCE_BARRIER::Transition(resource,
					D3D12_RESOURCE_STATE_COPY_DEST,
					static_cast<D3D12_RESOURCE_STATES>(buffer->m_states[frame_idx])));
		}
	}

	void UpdateByteAddressBuffer(HeapResource* buffer, unsigned int frame_idx, void* data, std::uint64_t size_in_bytes)
	{
		UpdateConstantBuffer(buffer, frame_idx, data, size_in_bytes);
	}

	void CreateSRVFromByteAddressBuffer(HeapResource* resource, DescHeapCPUHandle& handle, unsigned int id, unsigned int count)
	{
		auto& n_resources = resource->m_heap_bbo->m_resources[resource->m_heap_vector_location];
		auto& n_resource = n_resources.second[id];

		decltype(Device::m_native) n_device;
		n_resource->GetDevice(IID_PPV_ARGS(&n_device));

		auto increment_size = n_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.Format = DXGI_FORMAT_R32_TYPELESS;
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srv_desc.Buffer.NumElements = count;
		srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

		n_device->CreateShaderResourceView(n_resource, &srv_desc, handle.m_native);
		Offset(handle, 1, increment_size);
	}

} /* wr::d3d12 */
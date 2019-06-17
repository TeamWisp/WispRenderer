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

#include "d3d12_structs.hpp"
#include "d3d12_enums.hpp"

#include <cstdint>
#include <mutex>
#include <memory>
#include <set>
#include <map>
#include <vector>
#include <queue>

namespace wr
{
	//Forward declaration of a page, the DescriptorAllocation class needs it.
	class DescriptorAllocatorPage;

	//Forward declaration of the render system needed by the page
	class D3D12RenderSystem;

	///////////////////////////////
	//   DESCRIPTOR ALLOCATION   //
	///////////////////////////////

	//Represent a single allocation of contiguous descriptors in a descriptor heap.
	class DescriptorAllocation
	{
	public:
		// Creates a NULL descriptor.
		DescriptorAllocation();

		DescriptorAllocation(d3d12::DescHeapCPUHandle descriptor, 
							 uint32_t num_handles, 
							 uint32_t descriptor_size, 
							 std::shared_ptr<DescriptorAllocatorPage> page);

		// The destructor will automatically free the allocation.
		~DescriptorAllocation();

		// Only move is allowed.
		DescriptorAllocation(const DescriptorAllocation&) = delete;
		DescriptorAllocation& operator=(const DescriptorAllocation&) = delete;

		DescriptorAllocation(DescriptorAllocation&& allocation);
		DescriptorAllocation& operator=(DescriptorAllocation&& other);


		// Check if this a valid descriptor.
		bool IsNull() const;

		// Get a descriptor at a particular offset in the allocation.
		d3d12::DescHeapCPUHandle GetDescriptorHandle(uint32_t offset = 0) const;

		// Get the number of (consecutive) handles for this allocation.
		uint32_t GetNumHandles() const;

		// Get the heap that this allocation came from.
		// (For internal use only).
		std::shared_ptr<DescriptorAllocatorPage> GetDescriptorAllocatorPage() const;

	private:
		// Free the descriptor back to the heap it came from.
		void Free();

		// The base descriptor.
		d3d12::DescHeapCPUHandle m_descriptor;
		// The number of descriptors in this allocation.
		uint32_t m_num_handles;
		// The offset to the next descriptor.
		uint32_t m_descriptor_size;

		// A pointer back to the original page where this allocation came from.
		std::shared_ptr<DescriptorAllocatorPage> m_page;
	};

	///////////////////////////////
	// DESCRIPTOR ALLOCATOR PAGE //
	///////////////////////////////

	class DescriptorAllocatorPage : public std::enable_shared_from_this<DescriptorAllocatorPage>
	{
	public:
		DescriptorAllocatorPage(D3D12RenderSystem& render_system, DescriptorHeapType type, uint32_t num_descriptors);

		~DescriptorAllocatorPage();

		DescriptorHeapType GetHeapType() const;

		/**
		* Check to see if this descriptor page has a contiguous block of descriptors
		* large enough to satisfy the request.
		*/
		bool HasSpace(uint32_t num_descriptors) const;

		/**
		* Get the number of available handles in the heap.
		*/
		uint32_t NumFreeHandles() const;

		/**
		* Allocate a number of descriptors from this descriptor heap.
		* If the allocation cannot be satisfied, then a NULL descriptor
		* is returned.
		*/
		DescriptorAllocation Allocate(uint32_t num_descriptors);

		/**
		* Return a descriptor back to the heap.
		* Stale descriptors are not freed directly, but put
		* on a stale allocations queue. Stale allocations are returned to the heap
		* using the DescriptorAllocatorPage::ReleaseStaleAllocations method.
		*/
		void Free(DescriptorAllocation&& allocation_handle);

		/**
		* Returned the stale descriptors back to the descriptor heap.
		*/
		void ReleaseStaleDescriptors();

	protected:

		// Compute the offset of the descriptor handle from the start of the heap.
		uint32_t ComputeOffset(d3d12::DescHeapCPUHandle handle);

		// Adds a new block to the free list.
		void AddNewBlock(uint32_t offset, uint32_t num_descriptors);

		// Free a block of descriptors.
		// This will also merge free blocks in the free list to form larger blocks
		// that can be reused.
		void FreeBlock(uint32_t offset, uint32_t num_descriptors);

	private:
		// The offset (in descriptors) within the descriptor heap.
		using OffsetType = uint32_t;
		// The number of descriptors that are available.
		using SizeType = uint32_t;

		struct FreeBlockInfo;

		// A map that lists the free blocks by the offset within the descriptor heap.
		using FreeListByOffset = std::map<OffsetType, FreeBlockInfo>;

		// A map that lists the free blocks by size.
		// Needs to be a multimap since multiple blocks can have the same size.
		using FreeListBySize = std::multimap<SizeType, FreeListByOffset::iterator>;

		struct FreeBlockInfo
		{
			FreeBlockInfo(SizeType size)
				: m_size(size)
			{}

			SizeType m_size;
			FreeListBySize::iterator m_freelist_by_size_itr;
		};

		struct StaleDescriptorInfo
		{
			StaleDescriptorInfo(OffsetType offset, SizeType size, uint64_t frame)
				: m_offset(offset)
				, m_size(size)
				, m_frame_number(frame)
			{}

			// The offset within the descriptor heap.
			OffsetType m_offset;
			// The number of descriptors
			SizeType m_size;
			// The frame number that the descriptor was freed.
			uint64_t m_frame_number;
		};

		// Stale descriptors are queued for release until the frame that they were freed
		// has completed.
		using StaleDescriptorQueue = std::queue<StaleDescriptorInfo>;

		FreeListByOffset m_freelist_by_offset;
		FreeListBySize m_freelist_by_size;
		StaleDescriptorQueue m_stale_descriptors;

		d3d12::DescriptorHeap* m_descriptor_heap;
		DescriptorHeapType m_heap_type;
		d3d12::DescHeapCPUHandle m_base_descriptor;
		uint32_t m_num_descriptors_in_heap;
		uint32_t m_num_free_handles;

		std::mutex m_allocation_mutex;

		D3D12RenderSystem& m_render_system;
	};
}


//////////////////////////
// DESCRIPTOR ALLOCATOR //
//////////////////////////

namespace wr
{
	class DescriptorAllocator
	{
	public:
		DescriptorAllocator(D3D12RenderSystem& render_system, DescriptorHeapType type, uint32_t num_descriptors_per_heap = 256);
		virtual ~DescriptorAllocator();

		/**
		 * Allocate a number of contiguous descriptors from a CPU visible descriptor heap.
		 *
		 * @param numDescriptors The number of contiguous descriptors to allocate.
		 * Cannot be more than the number of descriptors per descriptor heap.
		 */
		DescriptorAllocation Allocate(uint32_t num_descriptors = 1);

		/**
		 * When the frame has completed, the stale descriptors can be released.
		 */
		void ReleaseStaleDescriptors();

	private:
		using DescriptorHeapPool = std::vector<std::shared_ptr<DescriptorAllocatorPage>>;

		// Create a new heap with a specific number of descriptors.
		std::shared_ptr<DescriptorAllocatorPage> CreateAllocatorPage();

		DescriptorHeapType m_heap_type;
		uint32_t m_num_descriptors_per_heap;

		DescriptorHeapPool m_heap_pool;

		// Indices of available heaps in the heap pool.
		std::set<size_t> m_available_heaps;

		std::mutex m_allocation_mutex;

		D3D12RenderSystem& m_render_system;
	};
}
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

#include "d3d12_descriptors_allocations.hpp"
#include "d3d12_renderer.hpp"
#include "d3d12_functions.hpp"
#include "../util/log.hpp"

// DESCRIPTOR ALLOCATION FUNCTIONS

namespace wr
{

	DescriptorAllocation::DescriptorAllocation()
		: m_descriptor{ 0 }
		, m_num_handles(0)
		, m_descriptor_size(0)
		, m_page(nullptr)
	{}

	DescriptorAllocation::DescriptorAllocation(d3d12::DescHeapCPUHandle descriptor,
		uint32_t num_handles,
		uint32_t descriptor_size,
		std::shared_ptr<DescriptorAllocatorPage> page)
		: m_descriptor(descriptor)
		, m_num_handles(num_handles)
		, m_descriptor_size(descriptor_size)
		, m_page(page)
	{}


	DescriptorAllocation::~DescriptorAllocation()
	{
		Free();
	}

	DescriptorAllocation::DescriptorAllocation(DescriptorAllocation&& allocation)
		: m_descriptor(allocation.m_descriptor)
		, m_num_handles(allocation.m_num_handles)
		, m_descriptor_size(allocation.m_descriptor_size)
		, m_page(std::move(allocation.m_page))
	{
		allocation.m_descriptor.m_native.ptr = 0;
		allocation.m_num_handles = 0;
		allocation.m_descriptor_size = 0;
	}

	DescriptorAllocation& DescriptorAllocation::operator=(DescriptorAllocation&& other)
	{
		// Free this descriptor if it points to anything.
		Free();

		m_descriptor = other.m_descriptor;
		m_num_handles = other.m_num_handles;
		m_descriptor_size = other.m_descriptor_size;
		m_page = std::move(other.m_page);

		other.m_descriptor.m_native.ptr = 0;
		other.m_num_handles = 0;
		other.m_descriptor_size = 0;

		return *this;
	}

	void DescriptorAllocation::Free()
	{
		if (!IsNull() && m_page)
		{
			m_page->Free(std::move(*this));
			m_descriptor.m_native.ptr = 0;
			m_num_handles = 0;
			m_descriptor_size = 0;
			m_page.reset();
		}
	}

	// Check if this a valid descriptor.
	bool DescriptorAllocation::IsNull() const
	{
		return m_descriptor.m_native.ptr == 0;
	}

	// Get a descriptor at a particular offset in the allocation.
	d3d12::DescHeapCPUHandle DescriptorAllocation::GetDescriptorHandle(uint32_t offset) const
	{
		if (offset > m_num_handles)
		{
			LOGC("Specified offset is greater than the number of handles in this Descriptor Allocation");
		}

		return { m_descriptor.m_native.ptr + (m_descriptor_size * offset) };
	}

	uint32_t DescriptorAllocation::GetNumHandles() const
	{
		return m_num_handles;
	}

	std::shared_ptr<DescriptorAllocatorPage> DescriptorAllocation::GetDescriptorAllocatorPage() const
	{
		return m_page;
	}
}

// DESCRIPTOR ALLOCATOR PAGE FUNCTIONS

namespace wr
{
	DescriptorAllocatorPage::DescriptorAllocatorPage(D3D12RenderSystem& render_system, DescriptorHeapType type, uint32_t num_descriptors)
		: m_render_system(render_system)
		, m_heap_type(type)
		, m_num_descriptors_in_heap(num_descriptors)
	{

		d3d12::desc::DescriptorHeapDesc desc;
		desc.m_type = m_heap_type;
		desc.m_num_descriptors = m_num_descriptors_in_heap;
		desc.m_versions = 1;
		desc.m_shader_visible = false;

		m_descriptor_heap = d3d12::CreateDescriptorHeap(m_render_system.m_device, desc);

		m_base_descriptor = d3d12::GetCPUHandle(m_descriptor_heap, 0);

		m_num_free_handles = m_num_descriptors_in_heap;

		// Initialize the free lists
		AddNewBlock(0, m_num_free_handles);
	}

	DescriptorAllocatorPage::~DescriptorAllocatorPage()
	{
		d3d12::Destroy(m_descriptor_heap);

		m_num_free_handles = 0;
		m_num_descriptors_in_heap = 0;

		m_freelist_by_offset.clear();
		m_freelist_by_size.clear();
	}

	DescriptorHeapType DescriptorAllocatorPage::GetHeapType() const
	{
		return m_heap_type;
	}

	uint32_t DescriptorAllocatorPage::NumFreeHandles() const
	{
		return m_num_free_handles;
	}

	bool DescriptorAllocatorPage::HasSpace(uint32_t num_descriptors) const
	{
		return m_freelist_by_size.lower_bound(num_descriptors) != m_freelist_by_size.end();
	}

	void DescriptorAllocatorPage::AddNewBlock(uint32_t offset, uint32_t num_descriptors)
	{
		auto offsetIt = m_freelist_by_offset.emplace(offset, num_descriptors);
		auto sizeIt = m_freelist_by_size.emplace(num_descriptors, offsetIt.first);
		offsetIt.first->second.m_freelist_by_size_itr = sizeIt;
	}

	DescriptorAllocation DescriptorAllocatorPage::Allocate(uint32_t num_descriptors)
	{
		std::lock_guard<std::mutex> lock(m_allocation_mutex);

		// There are less than the requested number of descriptors left in the heap.
		// Return a NULL descriptor and try another heap.
		if (num_descriptors > m_num_free_handles)
		{
			return DescriptorAllocation();
		}

		// Get the first block that is large enough to satisfy the request.
		auto smallestBlockIt = m_freelist_by_size.lower_bound(num_descriptors);
		if (smallestBlockIt == m_freelist_by_size.end())
		{
			// There was no free block that could satisfy the request.
			return DescriptorAllocation();
		}

		// The size of the smallest block that satisfies the request.
		auto blockSize = smallestBlockIt->first;

		// The pointer to the same entry in the FreeListByOffset map.
		auto offsetIt = smallestBlockIt->second;

		// The offset in the descriptor heap.
		auto offset = offsetIt->first;

		// Remove the existing free block from the free list.
		m_freelist_by_size.erase(smallestBlockIt);
		m_freelist_by_offset.erase(offsetIt);

		// Compute the new free block that results from splitting this block.
		auto newOffset = offset + num_descriptors;
		auto newSize = blockSize - num_descriptors;

		if (newSize > 0)
		{
			// If the allocation didn't exactly match the requested size,
			// return the left-over to the free list.
			AddNewBlock(newOffset, newSize);
		}

		// Decrement free handles.
		m_num_free_handles -= num_descriptors;

		d3d12::DescHeapCPUHandle new_handle;
		new_handle.m_native = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_base_descriptor.m_native, offset, m_descriptor_heap->m_increment_size);

		return DescriptorAllocation(new_handle, num_descriptors, m_descriptor_heap->m_increment_size, shared_from_this());
	}

	uint32_t DescriptorAllocatorPage::ComputeOffset(d3d12::DescHeapCPUHandle handle)
	{
		return static_cast<uint32_t>(handle.m_native.ptr - m_base_descriptor.m_native.ptr) / m_descriptor_heap->m_increment_size;
	}

	void DescriptorAllocatorPage::Free(DescriptorAllocation&& allocation_handle)
	{
		// Compute the offset of the descriptor within the descriptor heap.
		auto offset = ComputeOffset(allocation_handle.GetDescriptorHandle());

		std::lock_guard<std::mutex> lock(m_allocation_mutex);

		// Don't add the block directly to the free list until the frame has completed.
		m_stale_descriptors.emplace(offset, allocation_handle.GetNumHandles(), m_render_system.GetFrameIdx());
	}

	void DescriptorAllocatorPage::FreeBlock(uint32_t offset, uint32_t num_descriptors)
	{
		// Find the first element whose offset is greater than the specified offset.
		// This is the block that should appear after the block that is being freed.
		auto next_block_it = m_freelist_by_offset.upper_bound(offset);

		// Find the block that appears before the block being freed.
		auto prev_block_it = next_block_it;
		// If it's not the first block in the list.
		if (prev_block_it != m_freelist_by_offset.begin())
		{
			// Go to the previous block in the list.
			--prev_block_it;
		}
		else
		{
			// Otherwise, just set it to the end of the list to indicate that no
			// block comes before the one being freed.
			prev_block_it = m_freelist_by_offset.end();
		}

		// Add the number of free handles back to the heap.
		// This needs to be done before merging any blocks since merging
		// blocks modifies the numDescriptors variable.
		m_num_free_handles += num_descriptors;

		if (prev_block_it != m_freelist_by_offset.end() &&
			offset == prev_block_it->first + prev_block_it->second.m_size)
		{
			// The previous block is exactly behind the block that is to be freed.
			//
			// PrevBlock.Offset           Offset
			// |                          |
			// |<-----PrevBlock.Size----->|<------Size-------->|
			//

			// Increase the block size by the size of merging with the previous block.
			offset = prev_block_it->first;
			num_descriptors += prev_block_it->second.m_size;

			// Remove the previous block from the free list.
			m_freelist_by_size.erase(prev_block_it->second.m_freelist_by_size_itr);
			m_freelist_by_offset.erase(prev_block_it);
		}

		if (next_block_it != m_freelist_by_offset.end() &&
			offset + num_descriptors == next_block_it->first)
		{
			// The next block is exactly in front of the block that is to be freed.
			//
			// Offset               NextBlock.Offset 
			// |                    |
			// |<------Size-------->|<-----NextBlock.Size----->|

			// Increase the block size by the size of merging with the next block.
			num_descriptors += next_block_it->second.m_size;

			// Remove the next block from the free list.
			m_freelist_by_size.erase(next_block_it->second.m_freelist_by_size_itr);
			m_freelist_by_offset.erase(next_block_it);
		}

		// Add the freed block to the free list.
		AddNewBlock(offset, num_descriptors);
	}

	void DescriptorAllocatorPage::ReleaseStaleDescriptors()
	{
		std::lock_guard<std::mutex> lock(m_allocation_mutex);

		while (!m_stale_descriptors.empty() && m_stale_descriptors.front().m_frame_number <= m_render_system.GetFrameIdx())
		{
			auto& staleDescriptor = m_stale_descriptors.front();

			// The offset of the descriptor in the heap.
			auto offset = staleDescriptor.m_offset;
			// The number of descriptors that were allocated.
			auto numDescriptors = staleDescriptor.m_size;

			FreeBlock(offset, numDescriptors);

			m_stale_descriptors.pop();
		}
	}
}

// DESCRIPTOR ALLOCATOR FUNCTIONS

namespace wr
{
	DescriptorAllocator::DescriptorAllocator(D3D12RenderSystem& render_system, DescriptorHeapType type, uint32_t num_descriptors_per_heap)
		: m_render_system(render_system)
		, m_heap_type(type)
		, m_num_descriptors_per_heap(num_descriptors_per_heap)
	{
	}

	DescriptorAllocator::~DescriptorAllocator()
	{
		ReleaseStaleDescriptors();

		for (auto heap : m_heap_pool)
		{
			heap.reset();
		}
	}

	std::shared_ptr<DescriptorAllocatorPage> DescriptorAllocator::CreateAllocatorPage()
	{
		auto new_page = std::make_shared<DescriptorAllocatorPage>(m_render_system, m_heap_type, m_num_descriptors_per_heap);

		m_heap_pool.emplace_back(new_page);
		m_available_heaps.insert(m_heap_pool.size() - 1);

		return new_page;
	}

	DescriptorAllocation DescriptorAllocator::Allocate(uint32_t num_dscriptors)
	{
		std::lock_guard<std::mutex> lock(m_allocation_mutex);

		DescriptorAllocation allocation;

		for (auto iter = m_available_heaps.begin(); iter != m_available_heaps.end(); ++iter)
		{
			auto allocator_page = m_heap_pool[*iter];

			allocation = allocator_page->Allocate(num_dscriptors);

			if (allocator_page->NumFreeHandles() == 0)
			{
				iter = m_available_heaps.erase(iter);
			}

			// A valid allocation has been found.
			if (!allocation.IsNull())
			{
				break;
			}
		}

		// No available heap could satisfy the requested number of descriptors.
		if (allocation.IsNull())
		{
			m_num_descriptors_per_heap = std::max(m_num_descriptors_per_heap, num_dscriptors);
			auto new_page = CreateAllocatorPage();

			allocation = new_page->Allocate(num_dscriptors);
		}

		return allocation;
	}

	void DescriptorAllocator::ReleaseStaleDescriptors()
	{
		std::lock_guard<std::mutex> lock(m_allocation_mutex);

		for (size_t i = 0; i < m_heap_pool.size(); ++i)
		{
			auto page = m_heap_pool[i];

			page->ReleaseStaleDescriptors();

			if (page->NumFreeHandles() > 0)
			{
				m_available_heaps.insert(i);
			}
		}
	}




}
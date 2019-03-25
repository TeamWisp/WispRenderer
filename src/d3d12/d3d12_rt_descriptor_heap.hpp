#pragma once

#include "d3d12_structs.hpp"
#include "d3d12_enums.hpp"

#include <cstdint>
#include <memory>
#include <queue>



namespace wr
{
	namespace d3d12
	{
		struct Device;
	};
	
	class RTDescriptorHeap
	{
	public:
		RTDescriptorHeap(wr::d3d12::Device* device, DescriptorHeapType type, uint32_t num_descriptors_per_heap = 5000);

		virtual ~RTDescriptorHeap();

		/**
		 * Stages a contiguous range of CPU visible descriptors.
		 * Descriptors are not copied to the GPU visible descriptor heap until
		 * the CommitStagedDescriptors function is called.
		 */
		void StageDescriptors(uint32_t root_param_idx, uint32_t offset, uint32_t num_descriptors, const d3d12::DescHeapCPUHandle src_desc);

		/**
		 * Copy all of the staged descriptors to the GPU visible descriptor heap and
		 * bind the descriptor heap and the descriptor tables to the command list.
		 * The passed-in function object is used to set the GPU visible descriptors
		 * on the command list. Two possible functions are:
		 *   * Before a draw    : ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable
		 *   * Before a dispatch: ID3D12GraphicsCommandList::SetComputeRootDescriptorTable
		 */
		void CommitStagedDescriptorsForDraw(d3d12::CommandList& cmd_list, unsigned int frame_idx);
		void CommitStagedDescriptorsForDispatch(d3d12::CommandList& cmd_list, unsigned int frame_idx);

		/**
		 * Copies a single CPU visible descriptor to a GPU visible descriptor heap.
		 * This is useful for the
		 *   * ID3D12GraphicsCommandList::ClearUnorderedAccessViewFloat
		 *   * ID3D12GraphicsCommandList::ClearUnorderedAccessViewUint
		 * methods which require both a CPU and GPU visible descriptors for a UAV
		 * resource.
		 *
		 * @param commandList The command list is required in case the GPU visible
		 * descriptor heap needs to be updated on the command list.
		 * @param cpuDescriptor The CPU descriptor to copy into a GPU visible
		 * descriptor heap.
		 *
		 * @return The GPU visible descriptor.
		 */
		d3d12::DescHeapGPUHandle CopyDescriptor(d3d12::CommandList& cmd_list, d3d12::DescHeapCPUHandle cpu_desc, unsigned int frame_idx);

		/**
		 * Parse the root signature to determine which root parameters contain
		 * descriptor tables and determine the number of descriptors needed for
		 * each table.
		 */
		void ParseRootSignature(const d3d12::RootSignature& root_signature);

		/**
		 * Reset used descriptors. This should only be done if any descriptors
		 * that are being referenced by a command list has finished executing on the
		 * command queue.
		 */
		void Reset(unsigned int frame_idx);

		/**
		* Ideally, this is only used when building the acceleration structures.
		* Don't call it otherwise.
		*/
		d3d12::DescriptorHeap* GetHeap() { return m_heap; }

	protected:

	private:
		// Create a new descriptor heap of no descriptor heap is available.
		d3d12::DescriptorHeap* CreateDescriptorHeap();

		// Compute the number of stale descriptors that need to be copied
		// to GPU visible descriptor heap.
		uint32_t ComputeStaleDescriptorCount() const;

		/**
		 * The maximum number of descriptor tables per root signature.
		 * A 32-bit mask is used to keep track of the root parameter indices that
		 * are descriptor tables.
		 */
		static const uint32_t MaxDescriptorTables = 32;

		/**
		 * A structure that represents a descriptor table entry in the root signature.
		 */
		struct DescriptorTableCache
		{
			DescriptorTableCache()
				: m_num_descriptors(0)
				, m_base_descriptor(nullptr)
			{}

			// Reset the table cache.
			void Reset()
			{
				m_num_descriptors = 0;
				m_base_descriptor = nullptr;
			}

			// The number of descriptors in this descriptor table.
			uint32_t m_num_descriptors;
			// The pointer to the descriptor in the descriptor handle cache.
			D3D12_CPU_DESCRIPTOR_HANDLE* m_base_descriptor;
		};

		// Describes the type of descriptors that can be staged using this 
		// dynamic descriptor heap.
		// Valid values are:
		//   * D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		//   * D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
		// This parameter also determines the type of GPU visible descriptor heap to 
		// create.
		DescriptorHeapType m_desc_heap_type;

		// The number of descriptors to allocate in new GPU visible descriptor heaps.
		uint32_t m_num_descr_per_heap;

		uint32_t m_descriptor_handle_increment_size;

		// The descriptor handle cache.
		std::unique_ptr<D3D12_CPU_DESCRIPTOR_HANDLE[]> m_descriptor_handle_cache;

		// Descriptor handle cache per descriptor table.
		DescriptorTableCache m_descriptor_table_cache[MaxDescriptorTables];

		// Each bit in the bit mask represents the index in the root signature
		// that contains a descriptor table.
		uint32_t m_descriptor_table_bit_mask;

		// Each bit set in the bit mask represents a descriptor table
		// in the root signature that has changed since the last time the 
		// descriptors were copied.
		uint32_t m_stale_descriptor_table_bit_mask;

		d3d12::DescriptorHeap* m_heap;
		d3d12::DescHeapGPUHandle m_current_gpu_desc_handle;
		d3d12::DescHeapCPUHandle m_current_cpu_desc_handle;

		uint32_t m_num_free_handles[d3d12::settings::num_back_buffers];

		wr::d3d12::Device* m_device;
	};
}
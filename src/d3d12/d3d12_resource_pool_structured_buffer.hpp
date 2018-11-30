#pragma once

#include "../resource_pool_structured_buffer.hpp"
#include "d3d12_structs.hpp"

#include <queue>


namespace wr
{
	struct D3D12StructuredBufferHandle : StructuredBufferHandle
	{
		d3d12::HeapResource* m_native;
	};

	struct BufferUpdateInfo
	{
		D3D12StructuredBufferHandle* m_buffer_handle;
		std::vector<std::uint8_t> m_data;
		std::size_t m_offset;
		std::size_t m_size;
	};

	class D3D12RenderSystem;

	class D3D12StructuredBufferPool : public StructuredBufferPool
	{
	public:
		explicit D3D12StructuredBufferPool(D3D12RenderSystem& render_system,
			std::size_t size_in_mb);

		~D3D12StructuredBufferPool() final;

		void UpdateBuffers(d3d12::CommandList* cmd_list, std::size_t frame_idx);

		void Evict() final;
		void MakeResident() final;

	protected:
		[[nodiscard]] StructuredBufferHandle* CreateBuffer(std::size_t size, std::size_t stride, bool used_as_uav) final;
		void DestroyBuffer(StructuredBufferHandle* handle) final;
		void UpdateBuffer(StructuredBufferHandle* handle, void* data, std::size_t size, std::size_t offset) final;

		D3D12RenderSystem& m_render_system;

		d3d12::Heap<HeapOptimization::BIG_STATIC_BUFFERS>* m_heap;

		std::vector<D3D12StructuredBufferHandle*> m_handles;
		std::vector<std::queue<BufferUpdateInfo>> m_buffer_update_queues;
	};


} /* wr */
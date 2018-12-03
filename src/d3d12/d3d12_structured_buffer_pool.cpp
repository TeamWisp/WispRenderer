#include "d3d12_structured_buffer_pool.hpp"
#include "d3d12_functions.hpp"
#include "d3d12_renderer.hpp"

namespace wr
{
	D3D12StructuredBufferPool::D3D12StructuredBufferPool(D3D12RenderSystem & render_system, std::size_t size_in_mb) :
		StructuredBufferPool(size_in_mb),
		m_render_system(render_system)
	{
		m_heap = d3d12::CreateHeap_BSBO(m_render_system.m_device, size_in_mb*1024*1024, ResourceType::BUFFER, d3d12::settings::num_back_buffers);

		m_buffer_update_queues.resize(d3d12::settings::num_back_buffers);
	}

	D3D12StructuredBufferPool::~D3D12StructuredBufferPool()
	{
		for (D3D12StructuredBufferHandle* handle : m_handles) 
		{
			delete handle;
		}

		d3d12::Destroy(m_heap);
	}

	void D3D12StructuredBufferPool::UpdateBuffers(d3d12::CommandList * cmd_list, std::size_t frame_idx)
	{
		while (!m_buffer_update_queues[frame_idx].empty()) 
		{
			BufferUpdateInfo info = m_buffer_update_queues[frame_idx].front();
			d3d12::UpdateStructuredBuffer(info.m_buffer_handle->m_native,
				frame_idx,
				info.m_data.data(),
				info.m_size,
				info.m_offset,
				info.m_buffer_handle->m_native->m_stride,
				cmd_list);
			m_buffer_update_queues[frame_idx].pop();
		}
	}

	void D3D12StructuredBufferPool::Evict()
	{
		d3d12::Evict(m_heap);
	}

	void D3D12StructuredBufferPool::MakeResident()
	{
		d3d12::MakeResident(m_heap);
	}

	StructuredBufferHandle * D3D12StructuredBufferPool::CreateBuffer(std::size_t size, std::size_t stride, bool used_as_uav)
	{
		D3D12StructuredBufferHandle* handle = new D3D12StructuredBufferHandle();
		handle->m_pool = this;
		handle->m_native = d3d12::AllocStructuredBuffer(m_heap, size, stride, used_as_uav);

		if (handle->m_native == nullptr) 
		{
			delete handle;
			return nullptr;
		}

		m_handles.push_back(handle);

		return handle;
	}

	void D3D12StructuredBufferPool::DestroyBuffer(StructuredBufferHandle * handle)
	{
		std::vector<D3D12StructuredBufferHandle*>::iterator it;
		for (it = m_handles.begin(); it != m_handles.end(); ++it) 
		{
			if ((*it) == handle)
			{
				break;
			}
		}

		if (it == m_handles.end())
		{
			return;
		}

		m_handles.erase(it);

		d3d12::DeallocBuffer(m_heap, static_cast<D3D12StructuredBufferHandle*>(handle)->m_native);

		delete static_cast<D3D12StructuredBufferHandle*>(handle);
	}

	void D3D12StructuredBufferPool::UpdateBuffer(StructuredBufferHandle * handle, void * data, std::size_t size, std::size_t offset)
	{
		if (handle->m_pool == this) 
		{
			BufferUpdateInfo info = {};
			info.m_buffer_handle = static_cast<D3D12StructuredBufferHandle*>(handle);
			info.m_data.resize(size);
			memcpy(info.m_data.data(), data, size);
			info.m_size = size;
			info.m_offset = offset;

			for (int i = 0; i < d3d12::settings::num_back_buffers; ++i) 
			{
				m_buffer_update_queues[i].push(info);
			}
		}
	}

} /* wr */

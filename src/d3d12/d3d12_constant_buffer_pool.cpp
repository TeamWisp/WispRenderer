#include "d3d12_constant_buffer_pool.hpp"
#include "d3d12_functions.hpp"
#include "d3d12_settings.hpp"
#include "d3d12_renderer.hpp"

namespace wr 
{
	D3D12ConstantBufferPool::D3D12ConstantBufferPool(D3D12RenderSystem& render_system, std::size_t size_in_mb) : 
	ConstantBufferPool(size_in_mb),
	m_render_system(render_system)
	{
		m_heap = d3d12::CreateHeap_SBO(render_system.m_device, size_in_mb * 1024 * 1024, ResourceType::BUFFER, d3d12::settings::num_back_buffers);
		SetName(m_heap, L"Default SBO Heap");
		d3d12::MapHeap(m_heap);
	}

	D3D12ConstantBufferPool::~D3D12ConstantBufferPool()
	{
		d3d12::Destroy(m_heap);

		for (int i = 0; i < m_constant_buffer_handles.size(); ++i)
		{
			delete m_constant_buffer_handles[i];
		}
	}

	void D3D12ConstantBufferPool::Evict()
	{
		d3d12::Evict(m_heap);
	}

	void D3D12ConstantBufferPool::MakeResident()
	{
		d3d12::MakeResident(m_heap);
	}

	ConstantBufferHandle* D3D12ConstantBufferPool::AllocateConstantBuffer(std::size_t buffer_size)
	{
		D3D12ConstantBufferHandle* handle = new D3D12ConstantBufferHandle();
		handle->m_pool = this;
		handle->m_native = d3d12::AllocConstantBuffer(m_heap, buffer_size);
		if (handle->m_native == nullptr) 
		{
			delete handle;
			return nullptr;
		}

		m_constant_buffer_handles.push_back(handle);

		return handle;
	}

	void D3D12ConstantBufferPool::WriteConstantBufferData(ConstantBufferHandle * handle, size_t size, size_t offset, std::uint8_t * data)
	{
		auto frame_index = m_render_system.GetFrameIdx();
		std::uint8_t* cpu_address = static_cast<D3D12ConstantBufferHandle*>(handle)->m_native->m_cpu_addresses->operator[](frame_index);
		memcpy(cpu_address + offset, data, size);
	}

	void D3D12ConstantBufferPool::WriteConstantBufferData(ConstantBufferHandle * handle, size_t size, size_t offset, size_t frame_idx, std::uint8_t * data)
	{
		auto frame_index = frame_idx;
		std::uint8_t* cpu_address = static_cast<D3D12ConstantBufferHandle*>(handle)->m_native->m_cpu_addresses->operator[](frame_index);
		memcpy(cpu_address + offset, data, size);
	}

	void D3D12ConstantBufferPool::DeallocateConstantBuffer(ConstantBufferHandle* handle)
	{
		d3d12::DeallocConstantBuffer(m_heap, static_cast<D3D12ConstantBufferHandle*>(handle)->m_native);

		std::vector<ConstantBufferHandle*>::iterator it;
		for (it = m_constant_buffer_handles.begin(); it != m_constant_buffer_handles.end(); ++it)
		{
			if ((*it) == handle)
			{
				break;
			}
		}

		if (it != m_constant_buffer_handles.end())
		{
			m_constant_buffer_handles.erase(it);
			delete handle;
		}
	}
} /* wr */

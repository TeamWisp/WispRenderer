#include "constant_buffer_pool.hpp"

namespace wr 
{
	ConstantBufferPool::ConstantBufferPool(std::size_t size_in_bytes) : m_size_in_bytes(size_in_bytes)
	{
	}

	ConstantBufferHandle* ConstantBufferPool::Create(std::size_t buffer_size)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		return AllocateConstantBuffer(buffer_size);
	}

	void ConstantBufferPool::Update(ConstantBufferHandle* handle, size_t size, size_t offset, std::uint8_t * data)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		WriteConstantBufferData(handle, size, offset, data);
	}

	void ConstantBufferPool::Destroy(ConstantBufferHandle* handle)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		DeallocateConstantBuffer(handle);
	}
	void ConstantBufferPool::Update(ConstantBufferHandle * handle, size_t size, size_t offset, size_t frame_idx, std::uint8_t * data)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		WriteConstantBufferData(handle, size, offset, frame_idx, data);
	}
} /* wr */
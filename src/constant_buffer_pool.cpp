#include "constant_buffer_pool.hpp"

namespace wr 
{
	ConstantBufferPool::ConstantBufferPool(std::size_t size_in_mb) : m_size_in_mb(size_in_mb)
	{
	}

	ConstantBufferHandle* ConstantBufferPool::Create(std::size_t buffer_size)
	{
		return AllocateConstantBuffer(buffer_size);
	}

	void ConstantBufferPool::Update(ConstantBufferHandle* handle, size_t size, size_t offset, std::uint8_t * data)
	{
		WriteConstantBufferData(handle, size, offset, data);
	}

	void ConstantBufferPool::Destroy(ConstantBufferHandle* handle)
	{
		DeallocateConstantBuffer(handle);
	}
	void ConstantBufferPool::Update(ConstantBufferHandle * handle, size_t size, size_t offset, size_t frame_idx, std::uint8_t * data)
	{
		WriteConstantBufferData(handle, size, offset, frame_idx, data);
	}
} /* wr */
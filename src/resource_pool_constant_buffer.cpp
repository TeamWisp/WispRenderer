#include "resource_pool_constant_buffer.hpp"

namespace wr 
{
	ConstantBufferPool::ConstantBufferPool(std::size_t size_in_mb) : m_size_in_mb(size_in_mb)
	{
	}

	ConstantBufferHandle* ConstantBufferPool::Load(std::size_t buffer_size)
	{
		return AllocateConstantBuffer(buffer_size);
	}

	void ConstantBufferPool::Write(ConstantBufferHandle* handle, size_t size, size_t offset, std::uint8_t * data)
	{
		WriteConstantBufferData(handle, size, offset, data);
	}

	void ConstantBufferPool::Discard(ConstantBufferHandle* handle)
	{
		DeallocateConstantBuffer(handle);
	}
} /* wr */
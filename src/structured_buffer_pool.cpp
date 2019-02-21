#include "structured_buffer_pool.hpp"

namespace wr
{
	StructuredBufferPool::StructuredBufferPool(std::size_t size_in_bytes) :
		m_size_in_bytes(size_in_bytes)
	{
	}

	StructuredBufferHandle * StructuredBufferPool::Create(std::size_t size, std::size_t stride, bool used_as_uav)
	{
		return CreateBuffer(size, stride, used_as_uav);
	}

	void StructuredBufferPool::Destroy(StructuredBufferHandle * handle)
	{
		DestroyBuffer(handle);
	}

	void StructuredBufferPool::Update(StructuredBufferHandle * handle, void * data, std::size_t size, std::size_t offset)
	{
		UpdateBuffer(handle, data, size, offset);
	}

} /* wr */
#pragma once
#include <vector>

namespace wr
{
	class StructuredBufferPool;

	struct StructuredBufferHandle
	{
		StructuredBufferPool* m_pool;
	};

	class StructuredBufferPool
	{
	public:
		explicit StructuredBufferPool(std::size_t size_in_mb);
		virtual ~StructuredBufferPool() = default;

		StructuredBufferPool(StructuredBufferPool const &) = delete;
		StructuredBufferPool& operator=(StructuredBufferPool const &) = delete;
		StructuredBufferPool(StructuredBufferPool&&) = delete;
		StructuredBufferPool& operator=(StructuredBufferPool&&) = delete;

		[[nodiscard]] StructuredBufferHandle* Create(std::size_t size, std::size_t stride, bool used_as_uav);
		void Destroy(StructuredBufferHandle* handle);

		void Update(StructuredBufferHandle* handle, void* data, std::size_t size, std::size_t offset);

		virtual void Evict() = 0;
		virtual void MakeResident() = 0;

	protected:
		[[nodiscard]] virtual StructuredBufferHandle* CreateBuffer(std::size_t size, std::size_t stride, bool used_as_uav) = 0;
		virtual void DestroyBuffer(StructuredBufferHandle* handle) = 0;
		virtual void UpdateBuffer(StructuredBufferHandle* handle, void* data, std::size_t size, std::size_t offset) = 0;

		std::size_t m_size_in_mb;

	};


} /* wr */
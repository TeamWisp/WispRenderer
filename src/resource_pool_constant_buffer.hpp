#pragma once

#include <string_view>
#include <vector>
#include <optional>
#include <d3d12.h>

#include "util/defines.hpp"

namespace wr 
{
	class ConstantBufferPool;
	
	struct ConstantBufferHandle 
	{
		ConstantBufferPool* m_pool;
	};

	class ConstantBufferPool
	{
	public:
		explicit ConstantBufferPool(std::size_t size_in_mb);
		virtual ~ConstantBufferPool() = default;

		ConstantBufferPool(ConstantBufferPool const &) = delete;
		ConstantBufferPool& operator=(ConstantBufferPool const &) = delete;
		ConstantBufferPool(ConstantBufferPool&&) = delete;
		ConstantBufferPool& operator=(ConstantBufferPool&&) = delete;

		[[nodiscard]] ConstantBufferHandle* Create(std::size_t buffer_size);

		void Update(ConstantBufferHandle* handle, size_t size, size_t offset, std::uint8_t* data);
		void Update(ConstantBufferHandle* handle, size_t size, size_t offset, size_t frame_idx, std::uint8_t* data);

		void Destroy(ConstantBufferHandle* handle);

		virtual void Evict() = 0;
		virtual void MakeResident() = 0;

	protected:
		virtual ConstantBufferHandle* AllocateConstantBuffer(std::size_t buffer_size) = 0;
		virtual void WriteConstantBufferData(ConstantBufferHandle* handle, size_t size, size_t offset, std::uint8_t* data) = 0;
		virtual void WriteConstantBufferData(ConstantBufferHandle* handle, size_t size, size_t offset, size_t frame_idx, std::uint8_t* data) = 0;
		virtual void DeallocateConstantBuffer(ConstantBufferHandle* handle) = 0;

		std::size_t m_size_in_mb;
	};

} /* wr */

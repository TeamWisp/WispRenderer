/*!
 * Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <string_view>
#include <vector>
#include <optional>
#include <mutex>
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
		explicit ConstantBufferPool(std::size_t size_in_bytes);
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

		std::size_t m_size_in_bytes;
		std::mutex m_mutex;
	};

} /* wr */

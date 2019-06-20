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

#include <vector>
#include <mutex>

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
		explicit StructuredBufferPool(std::size_t size_in_bytes);
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

		std::size_t m_size_in_bytes;
		std::mutex m_mutex;
	};


} /* wr */
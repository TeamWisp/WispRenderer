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
#include "structured_buffer_pool.hpp"

namespace wr
{
	StructuredBufferPool::StructuredBufferPool(std::size_t size_in_bytes) :
		m_size_in_bytes(size_in_bytes)
	{
	}

	StructuredBufferHandle * StructuredBufferPool::Create(std::size_t size, std::size_t stride, bool used_as_uav)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		return CreateBuffer(size, stride, used_as_uav);
	}

	void StructuredBufferPool::Destroy(StructuredBufferHandle * handle)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		DestroyBuffer(handle);
	}

	void StructuredBufferPool::Update(StructuredBufferHandle * handle, void * data, std::size_t size, std::size_t offset)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		UpdateBuffer(handle, data, size, offset);
	}

} /* wr */
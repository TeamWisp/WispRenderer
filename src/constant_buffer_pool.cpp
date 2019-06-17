// Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
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

#include "../structured_buffer_pool.hpp"
#include "d3d12_structs.hpp"

#include <queue>


namespace wr
{
	struct D3D12StructuredBufferHandle : StructuredBufferHandle
	{
		d3d12::HeapResource* m_native;
	};

	struct BufferUpdateInfo
	{
		D3D12StructuredBufferHandle* m_buffer_handle;
		std::vector<std::uint8_t> m_data;
		std::size_t m_offset;
		std::size_t m_size;
		ResourceState m_new_state;
	};

	class D3D12RenderSystem;

	class D3D12StructuredBufferPool : public StructuredBufferPool
	{
	public:
		explicit D3D12StructuredBufferPool(D3D12RenderSystem& render_system,
			std::size_t size_in_bytes);

		~D3D12StructuredBufferPool() final;

		void UpdateBuffers(d3d12::CommandList* cmd_list, std::size_t frame_idx);

		void SetBufferState(StructuredBufferHandle* handle, ResourceState state);

		void Evict() final;
		void MakeResident() final;

	protected:
		[[nodiscard]] StructuredBufferHandle* CreateBuffer(std::size_t size, std::size_t stride, bool used_as_uav) final;
		void DestroyBuffer(StructuredBufferHandle* handle) final;
		void UpdateBuffer(StructuredBufferHandle* handle, void* data, std::size_t size, std::size_t offset) final;

		D3D12RenderSystem& m_render_system;

		d3d12::Heap<HeapOptimization::BIG_STATIC_BUFFERS>* m_heap;

		std::vector<D3D12StructuredBufferHandle*> m_handles;
		std::vector<std::queue<BufferUpdateInfo>> m_buffer_update_queues;
	};


} /* wr */
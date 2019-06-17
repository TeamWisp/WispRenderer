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

#pragma once
#include "../constant_buffer_pool.hpp"
#include "d3d12_structs.hpp"

namespace wr 
{

	struct D3D12ConstantBufferHandle : ConstantBufferHandle 
	{
		d3d12::HeapResource* m_native;
	};

	class D3D12RenderSystem;

	class D3D12ConstantBufferPool : public ConstantBufferPool 
	{
	public:
		explicit D3D12ConstantBufferPool(D3D12RenderSystem& render_system, std::size_t size_in_bytes);
		~D3D12ConstantBufferPool() final;

		void Evict() final;
		void MakeResident() final;
		
	protected:
		ConstantBufferHandle* AllocateConstantBuffer(std::size_t buffer_size) final;
		void WriteConstantBufferData(ConstantBufferHandle* handle, size_t size, size_t offset, std::uint8_t* data) final;
		void WriteConstantBufferData(ConstantBufferHandle* handle, size_t size, size_t offset, size_t frame_idx, std::uint8_t* data) final;
		void DeallocateConstantBuffer(ConstantBufferHandle* handle) final;

		std::vector<ConstantBufferHandle*> m_constant_buffer_handles;

		d3d12::Heap<HeapOptimization::SMALL_BUFFERS>* m_heap;
		D3D12RenderSystem& m_render_system;
	};

} /* wr */

#pragma once
#include "../resource_pool_constant_buffer.hpp"
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
		explicit D3D12ConstantBufferPool(D3D12RenderSystem* render_system, std::size_t size_in_mb);
		~D3D12ConstantBufferPool() final;

		void Evict() final;
		void MakeResident() final;
		
	protected:
		ConstantBufferHandle* AllocateConstantBuffer(std::size_t buffer_size) final;
		void WriteConstantBufferData(ConstantBufferHandle* handle, size_t size, size_t offset, std::uint8_t* data) final;
		void DeallocateConstantBuffer(ConstantBufferHandle* handle) final;

		std::vector<ConstantBufferHandle*> m_constant_buffer_handles;

		d3d12::Heap<HeapOptimization::SMALL_BUFFERS>* m_heap;
		D3D12RenderSystem* m_render_system;
	};

} /* wr */
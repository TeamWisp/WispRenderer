#include "d3d12_material_pool.hpp"

namespace wr
{
	D3D12MaterialPool::D3D12MaterialPool(D3D12RenderSystem & render_system) :
		m_render_system(render_system)
	{
		m_constant_buffer_pool = m_render_system.CreateConstantBufferPool(1 * 1024 * 1024);		
	}

	D3D12MaterialPool::~D3D12MaterialPool()
	{

	}

	void D3D12MaterialPool::Evict()
	{
		m_constant_buffer_pool->Evict();
	}

	void D3D12MaterialPool::MakeResident()
	{
		m_constant_buffer_pool->MakeResident();
	}

} /* wr */
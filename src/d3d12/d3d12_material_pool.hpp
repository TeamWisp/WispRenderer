#pragma once

#include "../material_pool.hpp"

#include "d3d12_renderer.hpp"

namespace wr
{

	class D3D12MaterialPool : public MaterialPool
	{
	public:
		explicit D3D12MaterialPool(D3D12RenderSystem& render_system);
		~D3D12MaterialPool() final;

		void Evict() final;
		void MakeResident() final;

	protected:
		D3D12RenderSystem& m_render_system;
	};

} /* wr */

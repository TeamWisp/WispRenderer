#pragma once

#include "../resource_pool_material.hpp"

namespace wr::d3d12
{
	struct HeapResource;
	struct StagingBuffer;
}

namespace wr 
{
	class D3D12RenderSystem;

	struct D3D12TextureHandle;

	struct D3D12MaterialHandle : MaterialHandle { };

	class D3D12MaterialPool : public MaterialPool
	{
	public:
		explicit D3D12MaterialPool(std::size_t size_in_mb);
		~D3D12MaterialPool() final;

		void Evict() final;
		void MakeResident() final;

	protected:
		MaterialHandle* LoadMaterial(std::string_view path) final;
		TextureHandle* LoadPNG(std::string_view path) final;
		TextureHandle* LoadDDS(std::string_view path) final;
		TextureHandle* LoadHDR(std::string_view path) final;
	};

}
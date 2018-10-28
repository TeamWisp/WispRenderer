#pragma once

#include "../resource_pool.hpp"

namespace wr
{
	struct D3D12TextureHandle : TextureHandle { };

	struct D3D12MaterialHandle : MaterialHandle { };

	class D3D12MaterialPool : public MaterialPool
	{
	public:
		explicit D3D12MaterialPool(std::size_t size_in_mb);
		virtual ~D3D12MaterialPool() final;
		
		virtual void Evict() final;
		virtual void MakeResident() final;
	
	protected:
		virtual MaterialHandle LoadMaterial(std::string_view path) final;
		virtual TextureHandle LoadPNG(std::string_view path) final;
		virtual TextureHandle LoadDDS(std::string_view path) final;
		virtual TextureHandle LoadHDR(std::string_view path) final;
	};

	class D3D12ModelPool : public ModelPool
	{
	public:
		explicit D3D12ModelPool(std::size_t size_in_mb);
		virtual ~D3D12ModelPool() final;

		virtual void Evict() final;
		virtual void MakeResident() final;

	protected:
		virtual ModelHandle LoadFBX(std::string_view path, ModelType type) final;
	};
}
#pragma once

#include "../resource_pool_texture.hpp"
#include "d3d12_structs.hpp"

namespace wr
{
	class D3D12RenderSystem;

	struct D3D12TextureHandle : TextureHandle
	{
		d3d12::Texture* m_native;
	};

	class D3D12TexturePool : public TexturePool
	{
	public:
		explicit D3D12TexturePool(D3D12RenderSystem& render_system, std::size_t size_in_mb, std::size_t num_of_textures);
		~D3D12TexturePool() final;

		void Evict() final;
		void MakeResident() final;
		void Stage(CommandList* cmd_list) final;
		void PostStageClear() final;


		d3d12::DescriptorHeap* GetHeap() { return m_descriptor_heap; }

	protected:

		D3D12TextureHandle* LoadPNG(std::string_view path, bool srgb) final;
		D3D12TextureHandle* LoadDDS(std::string_view path, bool srgb) final;
		D3D12TextureHandle* LoadHDR(std::string_view path, bool srgb) final;

		void LoadTextureResource(CommandList* cmd_list, d3d12::Texture* destination, ID3D12Resource* intermediate);

		D3D12RenderSystem& m_render_system;

		d3d12::DescriptorHeap* m_descriptor_heap;
		d3d12::DescHeapCPUHandle m_descriptor_handle;
	};




}
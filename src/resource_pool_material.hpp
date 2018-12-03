#pragma once

#include <string_view>
#include <vector>
#include <optional>
#include <d3d12.h>

#include "util/defines.hpp"
#include "resource_pool_texture.hpp"

struct aiMaterial;

namespace wr
{
	struct MaterialHandle {};
	
	class MaterialPool
	{
	public:
		explicit MaterialPool(std::size_t size_in_mb);
		virtual ~MaterialPool() = default;

		MaterialPool(MaterialPool const &) = delete;
		MaterialPool& operator=(MaterialPool const &) = delete;
		MaterialPool(MaterialPool&&) = delete;
		MaterialPool& operator=(MaterialPool&&) = delete;

		[[nodiscard]] MaterialHandle* Load(std::string_view path, TextureType type);
		[[nodiscard]] MaterialHandle* Load(aiMaterial* material);

		virtual void Evict() = 0;
		virtual void MakeResident() = 0;

	protected:
		virtual MaterialHandle* LoadMaterial(std::string_view path) = 0;
		virtual TextureHandle* LoadPNG(std::string_view path) = 0;
		virtual TextureHandle* LoadDDS(std::string_view path) = 0;
		virtual TextureHandle* LoadHDR(std::string_view path) = 0;

		std::size_t m_size_in_mb;
	};

} /* wr */
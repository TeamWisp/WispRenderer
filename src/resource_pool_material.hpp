#pragma once

#include <string_view>
#include <unordered_map>
#include <vector>
#include <optional>
#include <stdint.h>
#include <d3d12.h>

#include "structs.hpp"
#include "util/defines.hpp"
#include "id_factory.hpp"

struct aiMaterial;

namespace wr
{
	class MaterialPool
	{
	public:
		explicit MaterialPool();
		virtual ~MaterialPool() = default;

		MaterialPool(MaterialPool const &) = delete;
		MaterialPool& operator=(MaterialPool const &) = delete;
		MaterialPool(MaterialPool&&) = delete;
		MaterialPool& operator=(MaterialPool&&) = delete;

		//Creates an empty material. The user is responsible of filling in the texture handles.
		//TODO: Give Materials default textures
		[[nodiscard]] MaterialHandle Create();
		[[nodiscard]] MaterialHandle Create(TextureHandle& albedo,
											TextureHandle& normal,
											TextureHandle& rough_met,
											TextureHandle& ao, 
											bool is_alpha_masked = false, 
											bool is_double_sided = false);

		[[nodiscard]] MaterialHandle* Load(aiMaterial* material);

		virtual void Evict() {}
		virtual void MakeResident() {}

		virtual Material* GetMaterial(uint64_t material_id);

	protected:

		std::unordered_map<uint64_t, Material*> m_materials;

		IDFactory m_id_factory;
	};

} /* wr */
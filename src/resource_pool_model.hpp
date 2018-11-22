#pragma once

#include <string_view>
#include <vector>
#include <optional>
#include <d3d12.h>

#include "util/defines.hpp"
#include "resource_pool_material.hpp"

namespace wr
{
	struct Mesh
	{
	};

	template<typename TV, typename TI = std::uint32_t>
	struct MeshData
	{
		std::vector<TV> m_vertices;
		std::optional<std::vector<TI>> m_indices;
	};

	struct Model
	{
		std::vector<Mesh*> m_meshes;
	};

	enum class ModelType
	{
		FBX,
		CUSTOM
	};

	class ModelPool
	{
	public:
		explicit ModelPool(std::size_t size_in_mb);
		virtual ~ModelPool() = default;

		ModelPool(ModelPool const &) = delete;
		ModelPool& operator=(ModelPool const &) = delete;
		ModelPool(ModelPool&&) = delete;
		ModelPool& operator=(ModelPool&&) = delete;

		[[nodiscard]] Model* Load(std::string_view path, ModelType type);
		[[nodiscard]] std::pair<Model*, std::vector<MaterialHandle>> LoadWithMaterials(MaterialPool* material_pool, std::string_view path, ModelType type);
		template<typename TV, typename TI = std::uint32_t>
		[[nodiscard]] Model* LoadCustom(std::vector<MeshData<TV, TI>> meshes);

		virtual void Evict() = 0;
		virtual void MakeResident() = 0;

	protected:
		virtual Model* LoadFBX(std::string_view path, ModelType type) = 0;
		virtual Mesh* LoadCustom_VerticesAndIndices(void* vertices_data, std::size_t num_vertices, std::size_t vertex_size, void* indices_data, std::size_t num_indices, std::size_t index_size) = 0;
		virtual Mesh* LoadCustom_VerticesOnly(void* vertices_data, std::size_t num_vertices, std::size_t vertex_size) = 0;

		std::size_t m_size_in_mb;
	};

	template<typename TV, typename TI>
	Model* ModelPool::LoadCustom(std::vector<MeshData<TV, TI>> meshes)
	{
		IS_PROPER_VERTEX_CLASS(TV);

		auto model = new Model();

		for (auto& data : meshes)
		{
			if (data.m_indices.has_value())
			{
				model->m_meshes.push_back(LoadCustom_VerticesAndIndices(data.m_vertices.data(), data.m_vertices.size(), sizeof(TV), data.m_indices.value().data(), data.m_indices.value().size(), sizeof(TI)));
			}
			else
			{
				void* ptr = data.m_vertices.data();
				model->m_meshes.push_back(LoadCustom_VerticesOnly(ptr, data.m_vertices.size(), sizeof(TV)));
			}
		}

		return model;
	}

} /* wr */

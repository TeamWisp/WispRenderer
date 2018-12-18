#pragma once

#include <string_view>
#include <vector>
#include <optional>
#include <map>
#include <stack>
//#include <d3d12.h>

#include "util/defines.hpp"
#include "material_pool.hpp"
#include "resource_pool_texture.hpp"

#include "model_loader.hpp" 
#include "model_loader_assimp.hpp"

#include "util/log.hpp"
#include "vertex.hpp"

struct aiScene;
struct aiNode;

namespace wr
{
	class ModelPool;

	namespace internal 
	{
		struct MeshInternal
		{

		};
	}

	struct Mesh
	{
		std::uint64_t id;
	};

	template<typename TV, typename TI = std::uint32_t>
	struct MeshData
	{
		std::vector<TV> m_vertices;
		std::optional<std::vector<TI>> m_indices;
	};

	struct Model
	{
		std::vector<std::pair<Mesh*, MaterialHandle*>> m_meshes;
		ModelPool* m_model_pool;
		std::string m_model_name;
	};

	class ModelPool
	{
	public:
		explicit ModelPool(std::size_t vertex_buffer_pool_size_in_mb,
			std::size_t index_buffer_pool_size_in_mb);
		virtual ~ModelPool() = default;

		ModelPool(ModelPool const &) = delete;
		ModelPool& operator=(ModelPool const &) = delete;
		ModelPool(ModelPool&&) = delete;
		ModelPool& operator=(ModelPool&&) = delete;

		template<typename TV, typename TI = std::uint32_t>
		[[nodiscard]] Model* Load(MaterialPool* material_pool, TexturePool* texture_pool, std::string_view path);
		template<typename TV, typename TI = std::uint32_t>
		[[nodiscard]] Model* LoadWithMaterials(MaterialPool* material_pool, TexturePool* texture_pool, std::string_view path);
		template<typename TV, typename TI = std::uint32_t>
		[[nodiscard]] Model* LoadCustom(std::vector<MeshData<TV, TI>> meshes);

		void Destroy(Model* model);
		void Destroy(internal::MeshInternal* mesh);

		virtual void Evict() = 0;
		virtual void MakeResident() = 0;

	protected:
		virtual internal::MeshInternal* LoadCustom_VerticesAndIndices(void* vertices_data, std::size_t num_vertices, std::size_t vertex_size, void* indices_data, std::size_t num_indices, std::size_t index_size) = 0;
		virtual internal::MeshInternal* LoadCustom_VerticesOnly(void* vertices_data, std::size_t num_vertices, std::size_t vertex_size) = 0;

		virtual void DestroyModel(Model* model) = 0;
		virtual void DestroyMesh(internal::MeshInternal* mesh) = 0;

		template<typename TV, typename TI = std::uint32_t>
		int LoadNodeMeshes(ModelData* data, Model* model, MaterialHandle* default_material);
		template<typename TV, typename TI = std::uint32_t>
		int LoadNodeMeshesWithMaterials(ModelData* data, Model* model, std::vector<MaterialHandle*> materials);

		std::size_t m_vertex_buffer_pool_size_in_mb;
		std::size_t m_index_buffer_pool_size_in_mb;

		std::map<std::uint64_t, internal::MeshInternal*> m_loaded_meshes;
		std::stack<std::uint64_t> m_freed_ids;

		std::uint64_t m_current_id;

		std::uint64_t GetNewID();
		void FreeID(std::uint64_t id);

	};

	template<typename TV, typename TI>
	Model* ModelPool::LoadCustom(std::vector<MeshData<TV, TI>> meshes)
	{
		IS_PROPER_VERTEX_CLASS(TV);

		auto model = new Model();

		for (int i = 0; i < meshes.size(); ++i)
		{
			Mesh* mesh = new Mesh();
			if (meshes[i].m_indices.has_value())
			{
				internal::MeshInternal* mesh_data = LoadCustom_VerticesAndIndices(
					meshes[i].m_vertices.data(),
					meshes[i].m_vertices.size(),
					sizeof(TV),
					meshes[i].m_indices.value().data(),
					meshes[i].m_indices.value().size(),
					sizeof(TI));

				std::uint64_t id = GetNewID();
				m_loaded_meshes[id] = mesh_data;
				mesh->id = id;
			}
			else
			{
				internal::MeshInternal* mesh_data = LoadCustom_VerticesOnly(
					meshes[i].m_vertices.data(),
					meshes[i].m_vertices.size(),
					sizeof(TV));

				std::uint64_t id = GetNewID();
				m_loaded_meshes[id] = mesh_data;
				mesh->id = id;
			}
			model->m_meshes.push_back(
				std::make_pair(mesh, nullptr));
		}

		model->m_model_pool = this;

		return model;
	}

	//! Loads a model without materials
	template<typename TV, typename TI>
	Model* ModelPool::Load(MaterialPool* material_pool, TexturePool* texture_pool, std::string_view path)
	{
		IS_PROPER_VERTEX_CLASS(TV);

		ModelLoader* loader = ModelLoader::FindFittingModelLoader(
			path.substr(path.find_last_of(".") + 1).data());

		if (loader == nullptr)
		{
			return nullptr;
		}

		ModelData* data = loader->Load(path);

		Model* model = new Model;

		MaterialHandle* default_material = nullptr;

		// TODO: Create default material

		int ret = LoadNodeMeshes<TV, TI>(data, model, default_material);

		if (ret == 1)
		{
			DestroyModel(model);

			loader->DeleteModel(data);

			return nullptr;
		}

		loader->DeleteModel(data);

		model->m_model_name = path.data();
		model->m_model_pool = this;

		return model;
	}

	//! Loads a model with materials
	template<typename TV, typename TI>
	Model* ModelPool::LoadWithMaterials(MaterialPool* material_pool, TexturePool* texture_pool, std::string_view path)
	{
		IS_PROPER_VERTEX_CLASS(TV);

		ModelLoader* loader = ModelLoader::FindFittingModelLoader(
			path.substr(path.find_last_of(".") + 1).data());

		if (loader == nullptr)
		{
			return nullptr;
		}

		ModelData* data = loader->Load(path);

		Model* model = new Model;
		std::vector<MaterialHandle*> material_handles;

		for (int i = 0; i < data->m_materials.size(); ++i)
		{
			TextureHandle albedo, normals, metallic, roughness, ambient_occlusion;

			ModelMaterialData* material = data->m_materials[i];

			if (material->m_albedo_texture_location!=TextureLocation::NON_EXISTENT)
			{
				if (material->m_albedo_texture_location==TextureLocation::EMBEDDED)
				{
					EmbeddedTexture* texture = data->m_embedded_textures[material->m_albedo_embedded_texture];

					// TODO: Actually load embeded texture
				}
				else if(material->m_albedo_texture_location==TextureLocation::EXTERNAL)
				{
					albedo = texture_pool->Load(material->m_albedo_texture);
				}
			}
			else
			{
				// TODO: Set texture to default texture
			}

			if (material->m_normal_map_texture_location != TextureLocation::NON_EXISTENT)
			{
				if (material->m_normal_map_texture_location == TextureLocation::EMBEDDED)
				{
					EmbeddedTexture* texture = data->m_embedded_textures[material->m_normal_map_embedded_texture];

					// TODO: Actually load embeded texture
				}
				else if (material->m_normal_map_texture_location == TextureLocation::EXTERNAL)
				{
					normals = texture_pool->Load(material->m_normal_map_texture);
				}
			}
			else
			{
				// TODO: Set texture to default texture
			}

			if (material->m_metallic_texture_location != TextureLocation::NON_EXISTENT)
			{
				if (material->m_metallic_texture_location == TextureLocation::EMBEDDED)
				{
					EmbeddedTexture* texture = data->m_embedded_textures[material->m_metallic_embedded_texture];

					// TODO: Actually load embeded texture
				}
				else if (material->m_metallic_texture_location == TextureLocation::EXTERNAL)
				{
					metallic = texture_pool->Load(material->m_metallic_texture);
				}
			}
			else
			{
				// TODO: Set texture to default texture
			}

			if (material->m_roughness_texture_location != TextureLocation::NON_EXISTENT)
			{
				if (material->m_roughness_texture_location == TextureLocation::EMBEDDED)
				{
					EmbeddedTexture* texture = data->m_embedded_textures[material->m_roughness_embedded_texture];

					// TODO: Actually load embeded texture
				}
				else if (material->m_roughness_texture_location == TextureLocation::EXTERNAL)
				{
					roughness = texture_pool->Load(material->m_roughness_texture);
				}
			}
			else
			{
				// TODO: Set texture to default texture
			}

			if (material->m_ambient_occlusion_texture_location != TextureLocation::NON_EXISTENT)
			{
				if (material->m_ambient_occlusion_texture_location == TextureLocation::EMBEDDED)
				{
					EmbeddedTexture* texture = data->m_embedded_textures[material->m_ambient_occlusion_embedded_texture];

					// TODO: Actually load embeded texture
				}
				else if (material->m_ambient_occlusion_texture_location == TextureLocation::EXTERNAL)
				{
					ambient_occlusion = texture_pool->Load(material->m_ambient_occlusion_texture);
				}
			}
			else
			{
				// TODO: Set texture to default texture
			}

			bool two_sided = material->m_two_sided;

			float opacity = material->m_base_transparency;

			material_pool->Create(albedo, normals, metallic, ambient_occlusion, opacity < 1.f, two_sided);
			material_handles.push_back();
		}

		int ret = LoadNodeMeshesWithMaterials<TV, TI>(data, model, material_handles);

		if (ret == 1)
		{
			DestroyModel(model);
			loader->DeleteModel(data);
			return nullptr;
		}

		loader->DeleteModel(data);

		model->m_model_name = path.data();
		model->m_model_pool = this;

		return model;
	}

	
} /* wr */

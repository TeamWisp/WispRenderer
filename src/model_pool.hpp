#pragma once

#include <string_view>
#include <vector>
#include <optional>
#include <map>
#include <stack>
//#include <d3d12.h>
#include <DirectXMath.h>

#include "util/defines.hpp"
#include "material_pool.hpp"
#include "resource_pool_texture.hpp"

#include "model_loader.hpp" 
#include "model_loader_assimp.hpp"

#include "util/log.hpp"
#include "util/aabb.hpp"
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
		//Box m_box;
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

		Box m_box;

		void Expand(float (&pos)[3], Mesh *mesh);

	};

	class ModelPool
	{
	public:
		explicit ModelPool(std::size_t vertex_buffer_pool_size_in_bytes,
			std::size_t index_buffer_pool_size_in_bytes);
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

		// Shrinks down both heaps to the minimum size required. 
		// Does not rearrange the contents of the heaps, meaning that it doesn't shrink to the absolute minimum size.
		// To do that, call Defragment first.
		virtual void ShrinkToFit() = 0;
		virtual void ShrinkVertexHeapToFit() = 0;
		virtual void ShrinkIndexHeapToFit() = 0;

		// Removes any holes in the memory, stitching all allocations back together to maximize the amount of contiguous free space.
		// These functions are called automatically if the allocator has enough free space but no large enough free blocks.
		virtual void Defragment() = 0;
		virtual void DefragmentVertexHeap() = 0;
		virtual void DefragmentIndexHeap() = 0;

		virtual size_t GetVertexHeapOccupiedSpace() = 0;
		virtual size_t GetIndexHeapOccupiedSpace() = 0;

		virtual size_t GetVertexHeapFreeSpace() = 0;
		virtual size_t GetIndexHeapFreeSpace() = 0;

		virtual size_t GetVertexHeapSize() = 0;
		virtual size_t GetIndexHeapSize() = 0;

		// Resizes both heaps to the supplied sizes. 
		// If the supplied size is smaller than the required size the heaps will resize to the required size instead.
		virtual void Resize(size_t vertex_heap_new_size, size_t index_heap_new_size) = 0;

		template<typename TV, typename TI> void EditMesh(Mesh* mesh, std::vector<TV> vertices, std::vector<TI> indices);


		virtual void Evict() = 0;
		virtual void MakeResident() = 0;

	protected:
		virtual internal::MeshInternal* LoadCustom_VerticesAndIndices(void* vertices_data, std::size_t num_vertices, std::size_t vertex_size, void* indices_data, std::size_t num_indices, std::size_t index_size) = 0;
		virtual internal::MeshInternal* LoadCustom_VerticesOnly(void* vertices_data, std::size_t num_vertices, std::size_t vertex_size) = 0;

		virtual void UpdateMeshData(Mesh* mesh, void* vertices_data, std::size_t num_vertices, std::size_t vertex_size, void* indices_data, std::size_t num_indices, std::size_t index_size) = 0;

		virtual void DestroyModel(Model* model) = 0;
		virtual void DestroyMesh(internal::MeshInternal* mesh) = 0;

		template<typename TV, typename TI = std::uint32_t>
		int LoadNodeMeshes(ModelData* data, Model* model, MaterialHandle* default_material);
		template<typename TV, typename TI = std::uint32_t>
		int LoadNodeMeshesWithMaterials(ModelData* data, Model* model, std::vector<MaterialHandle*> materials);

		std::size_t m_vertex_buffer_pool_size_in_bytes;
		std::size_t m_index_buffer_pool_size_in_bytes;

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

			if constexpr (std::is_same<TV, Vertex>::value || std::is_same<TV, VertexNoTangent>::value)
			{
				for (uint32_t j = 0, k = (uint32_t) meshes[i].m_vertices.size(); j < k; ++j)
				{
					model->Expand(meshes[i].m_vertices[j].m_pos, mesh);
				}
			}

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

		// Find directory
		std::string dir = std::string(path);
		dir.erase(dir.begin() + dir.find_last_of('/') + 1, dir.end());

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
					albedo = texture_pool->Load(dir + material->m_albedo_texture, false, false);
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
					normals = texture_pool->Load(dir + material->m_normal_map_texture, false, false);
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
					metallic = texture_pool->Load(dir + material->m_metallic_texture, false, false);
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
					roughness = texture_pool->Load(dir + material->m_roughness_texture, false, false);
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
					ambient_occlusion = texture_pool->Load(dir + material->m_ambient_occlusion_texture, false, false);
				}
			}
			else
			{
				// TODO: Set texture to default texture
			}

			bool two_sided = material->m_two_sided;

			float opacity = material->m_base_transparency;

			auto new_handle = material_pool->Create(albedo, normals, roughness, metallic, ambient_occlusion, false, true);
			material_handles.push_back(new_handle);
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

	template<typename TV, typename TI>
	void ModelPool::EditMesh(Mesh* mesh, std::vector<TV> vertices, std::vector<TI> indices)
	{
		UpdateMeshData(mesh,
			vertices.data(),
			vertices.size(),
			sizeof(TV),
			indices.data(),
			indices.size(),
			sizeof(TI));
	}
	
} /* wr */

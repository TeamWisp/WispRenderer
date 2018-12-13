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

#undef min
#undef max
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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

		DirectX::XMVECTOR m_aabb[2] = {
			{
				std::numeric_limits<float>::max(),
				std::numeric_limits<float>::max(),
				std::numeric_limits<float>::max()
			},
			{
				-std::numeric_limits<float>::max(),
				-std::numeric_limits<float>::max(),
				-std::numeric_limits<float>::max()
			}
		};

		void CalculateAABB(float (&pos)[3]);

	};

	enum class ModelType
	{
		FBX,
		CUSTOM
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
		[[nodiscard]] Model* Load(std::string_view path, ModelType type);
		template<typename TV, typename TI = std::uint32_t>
		[[nodiscard]] std::pair<Model*, std::vector<MaterialHandle*>> LoadWithMaterials(MaterialPool* material_pool, std::string_view path, ModelType type);
		template<typename TV, typename TI = std::uint32_t>
		[[nodiscard]] Model* LoadCustom(std::vector<MeshData<TV, TI>> meshes);

		void Destroy(Model* model);
		void Destroy(internal::MeshInternal* mesh);

		virtual void Evict() = 0;
		virtual void MakeResident() = 0;

	protected:
		virtual Model* LoadFBX(std::string_view path, ModelType type) = 0;
		virtual internal::MeshInternal* LoadCustom_VerticesAndIndices(void* vertices_data, std::size_t num_vertices, std::size_t vertex_size, void* indices_data, std::size_t num_indices, std::size_t index_size) = 0;
		virtual internal::MeshInternal* LoadCustom_VerticesOnly(void* vertices_data, std::size_t num_vertices, std::size_t vertex_size) = 0;

		virtual void DestroyModel(Model* model) = 0;
		virtual void DestroyMesh(internal::MeshInternal* mesh) = 0;

		template<typename TV, typename TI = std::uint32_t>
		int LoadNodeMeshes(const aiScene* scene, aiNode* node, Model* model);
		template<typename TV, typename TI = std::uint32_t>
		int LoadNodeMeshesWithMaterials(const aiScene* scene, aiNode* node, Model* model, std::vector<MaterialHandle*> materials);

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

			if constexpr (std::is_same<TV, Vertex>::value || std::is_same<TV, VertexNoTangent>::value)
			{
				for (uint32_t j = 0, k = (uint32_t) meshes[i].m_vertices.size(); j < k; ++j)
				{
					model->CalculateAABB(meshes[i].m_vertices[j].m_pos);
				}
			}

		}

		model->m_model_pool = this;

		return model;
	}

	//! Loads a model without materials
	template<typename TV, typename TI>
	Model* ModelPool::Load(std::string_view path, ModelType type)
	{
		IS_PROPER_VERTEX_CLASS(TV)

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path.data(),
			aiProcess_Triangulate |
			aiProcess_CalcTangentSpace |
			aiProcess_JoinIdenticalVertices |
			aiProcess_OptimizeMeshes |
			aiProcess_ImproveCacheLocality |
			aiProcess_MakeLeftHanded);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			LOGW(std::string("Loading model ") +
				path.data() +
				std::string(" failed with error ") +
				importer.GetErrorString());
			return nullptr;
		}

		Model* model = new Model;

		int ret = LoadNodeMeshes<TV, TI>(scene, scene->mRootNode, model);

		if (ret == 1)
		{
			DestroyModel(model);
			return nullptr;
		}
		model->m_model_name = path.data();
		model->m_model_pool = this;

		return model;
	}

	//! Loads a model with materials
	template<typename TV, typename TI>
	std::pair<Model*, std::vector<MaterialHandle*>> ModelPool::LoadWithMaterials(MaterialPool* material_pool, std::string_view path, ModelType type)
	{
		IS_PROPER_VERTEX_CLASS(TV)

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path.data(),
			aiProcess_Triangulate |
			aiProcess_CalcTangentSpace |
			aiProcess_JoinIdenticalVertices |
			aiProcess_OptimizeMeshes |
			aiProcess_ImproveCacheLocality |
			aiProcess_MakeLeftHanded);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			LOGW(std::string("Loading model ") +
				path.data() +
				std::string(" failed with error ") +
				importer.GetErrorString());
			return std::pair<Model*, std::vector<MaterialHandle*>>();
		}

		Model* model = new Model;
		std::vector<MaterialHandle*> material_handles;

		for (int i = 0; i < scene->mNumMaterials; ++i)
		{
			material_handles.push_back(material_pool->Load(scene->mMaterials[i]));
		}

		int ret = LoadNodeMeshesWithMaterials<TV, TI>(scene, scene->mRootNode, model, material_handles);

		if (ret == 1)
		{
			DestroyModel(model);
			return std::pair<Model*, std::vector<MaterialHandle*>>(nullptr, {});
		}

		model->m_model_name = path.data();
		model->m_model_pool = this;

		return std::pair<Model*, std::vector<MaterialHandle*>>(model, material_handles);
	}

	
} /* wr */

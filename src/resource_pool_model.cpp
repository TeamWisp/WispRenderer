#define NOMINMAX 1
#include "resource_pool_model.hpp"

 #include <assimp/Importer.hpp>
 #include <assimp/scene.h>
 #include <assimp/postprocess.h>
#include "util/log.hpp"
#include "vertex.hpp"

namespace wr
{
	ModelPool::ModelPool(std::size_t vertex_buffer_pool_size_in_mb,
		std::size_t index_buffer_pool_size_in_mb) : 
		m_vertex_buffer_pool_size_in_mb(vertex_buffer_pool_size_in_mb),
		m_index_buffer_pool_size_in_mb(index_buffer_pool_size_in_mb)
	{
	}

	//! Loads a model without materials
	Model* ModelPool::Load(std::string_view path, ModelType type)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path.data(),
			aiProcess_Triangulate |
			aiProcess_CalcTangentSpace |
			aiProcess_JoinIdenticalVertices |
			aiProcess_OptimizeMeshes |
			aiProcess_ImproveCacheLocality);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			LOGW(std::string("Loading model ") +
				path.data() + 
				std::string(" failed with error ") + 
				importer.GetErrorString());
			return nullptr;
		}

		Model* model = new Model;

		int ret = LoadNodeMeshes(scene, scene->mRootNode, model);

		if (ret == 1) 
		{
			DestroyModel(model);
			return nullptr;
		}

		return model;

		return new Model();
	}

	//! Loads a model with materials
	std::pair<Model*, std::vector<MaterialHandle*>> ModelPool::LoadWithMaterials(MaterialPool* material_pool, std::string_view path, ModelType type)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path.data(),
			aiProcess_Triangulate |
			aiProcess_CalcTangentSpace |
			aiProcess_JoinIdenticalVertices |
			aiProcess_OptimizeMeshes |
			aiProcess_ImproveCacheLocality);

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

		int ret = LoadNodeMeshesWithMaterials(scene, scene->mRootNode, model, material_handles);

		if (ret == 1) 
		{
			DestroyModel(model);
			return std::pair<Model*, std::vector<MaterialHandle*>>(nullptr, {});
		}

		return std::pair<Model*, std::vector<MaterialHandle*>>();
	}

	void ModelPool::Destroy(Model * model)
	{
		DestroyModel(model);
	}

	void ModelPool::Destroy(Mesh * mesh)
	{
		DestroyMesh(mesh);
	}

	int ModelPool::LoadNodeMeshes(const aiScene * scene, aiNode * node, Model* model)
	{
		for (unsigned int i = 0; i < node->mNumMeshes; ++i) 
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			std::vector<wr::Vertex> vertices;
			std::vector<std::uint32_t> indices;

			for (unsigned int j = 0; j < mesh->mNumVertices; ++j) 
			{
				Vertex vertex = {};

				vertex.m_pos[0] = mesh->mVertices[j].x;
				vertex.m_pos[1] = mesh->mVertices[j].y;
				vertex.m_pos[2] = mesh->mVertices[j].z;

				if (mesh->mNormals)
				{
					vertex.m_normal[0] = mesh->mNormals[j].x;
					vertex.m_normal[1] = mesh->mNormals[j].y;
					vertex.m_normal[2] = mesh->mNormals[j].z;
				}

				if (mesh->mTextureCoords[0] > 0) 
				{
					vertex.m_uv[0] = mesh->mTextureCoords[0][j].x;
					vertex.m_uv[1] = mesh->mTextureCoords[0][j].y;
				}

				if(mesh->mTangents)
				{
					vertex.m_tangent[0] = mesh->mTangents[j].x;
					vertex.m_tangent[1] = mesh->mTangents[j].y;
					vertex.m_tangent[2] = mesh->mTangents[j].z;

					vertex.m_bitangent[0] = mesh->mBitangents[j].x;
					vertex.m_bitangent[1] = mesh->mBitangents[j].y;
					vertex.m_bitangent[2] = mesh->mBitangents[j].z;
				}

				vertices.push_back(vertex);
			}

			for (size_t j = 0; j < mesh->mNumFaces; ++j)
			{
				aiFace *face = &mesh->mFaces[j];

				// retrieve all indices of the face and store them in the indices vector
				for (size_t k = 0; k < face->mNumIndices; k++)
				{
					indices.push_back(static_cast<unsigned>(face->mIndices[j]));
				}
			}

			Mesh* n_mesh = LoadCustom_VerticesAndIndices(
				vertices.data(),
				vertices.size(),
				sizeof(wr::Vertex),
				indices.data(),
				indices.size(),
				sizeof(std::uint32_t));

			if (n_mesh == nullptr) 
			{
				return 1;
			}

			model->m_meshes.push_back(n_mesh);
		}
		for (unsigned int i = 0; i < node->mNumChildren; ++i) 
		{
			int ret = LoadNodeMeshes(scene, node->mChildren[i], model);
			if (ret == 1)
				return 1;
		}
		return 0;
	}

	int ModelPool::LoadNodeMeshesWithMaterials(const aiScene * scene, aiNode * node, Model * model, std::vector<MaterialHandle*> materials)
	{
		for (unsigned int i = 0; i < node->mNumMeshes; ++i)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			std::vector<wr::Vertex> vertices;
			std::vector<std::uint32_t> indices;

			for (unsigned int j = 0; j < mesh->mNumVertices; ++j)
			{
				Vertex vertex = {};

				vertex.m_pos[0] = mesh->mVertices[j].x;
				vertex.m_pos[1] = mesh->mVertices[j].y;
				vertex.m_pos[2] = mesh->mVertices[j].z;

				if (mesh->mNormals)
				{
					vertex.m_normal[0] = mesh->mNormals[j].x;
					vertex.m_normal[1] = mesh->mNormals[j].y;
					vertex.m_normal[2] = mesh->mNormals[j].z;
				}

				if (mesh->mTextureCoords[0] > 0) 
				{
					vertex.m_uv[0] = mesh->mTextureCoords[0][j].x;
					vertex.m_uv[1] = mesh->mTextureCoords[0][j].y;
				}
				
				if (mesh->mTangents)
				{
					vertex.m_tangent[0] = mesh->mTangents[j].x;
					vertex.m_tangent[1] = mesh->mTangents[j].y;
					vertex.m_tangent[2] = mesh->mTangents[j].z;

					vertex.m_bitangent[0] = mesh->mBitangents[j].x;
					vertex.m_bitangent[1] = mesh->mBitangents[j].y;
					vertex.m_bitangent[2] = mesh->mBitangents[j].z;
				}

				vertices.push_back(vertex);
			}

			for (size_t j = 0; j < mesh->mNumFaces; ++j)
			{
				aiFace *face = &mesh->mFaces[j];

				// retrieve all indices of the face and store them in the indices vector
				for (size_t k = 0; k < face->mNumIndices; k++)
				{
					indices.push_back(static_cast<unsigned>(face->mIndices[j]));
				}
			}

			Mesh* n_mesh = LoadCustom_VerticesAndIndices(
				vertices.data(),
				vertices.size(),
				sizeof(wr::Vertex),
				indices.data(),
				indices.size(),
				sizeof(std::uint32_t));

			if (n_mesh == nullptr) 
			{
				return 1;
			}

			if (scene->HasMaterials())
			{
				n_mesh->material = materials[mesh->mMaterialIndex];
			}

			model->m_meshes.push_back(n_mesh);
		}
		for (unsigned int i = 0; i < node->mNumChildren; ++i)
		{
			int ret = LoadNodeMeshes(scene, node->mChildren[i], model);
			if (ret == 1)
				return 1;
		}
		return 0;
	}

} /* wr */